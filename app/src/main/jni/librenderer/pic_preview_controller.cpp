#include "./pic_preview_controller.h"

#define LOG_TAG "PicPreviewController"

PicPreviewController::PicPreviewController() :
		_msg(MSG_NONE), previewSurface(0), eglCore(0) {
	LOGI("VideoDutePlayerController instance created");
	pthread_mutex_init(&mLock, NULL); //互斥锁的初始化，函数成功执行后，互斥锁被初始化为未锁住态，attr为锁属性，NULL值为默认属性
	pthread_cond_init(&mCondition, NULL);//初始化一个条件变量。当参数cattr为空指针时，函数创建的是一个缺省的条件变量。否则条件变量的属性将由cattr中的属性值来决定。
	renderer = new PicPreviewRender();
	this->screenWidth = 720;
	this->screenHeight = 720;
	decoder = NULL;
}

PicPreviewController::~PicPreviewController() {
	LOGI("VideoDutePlayerController instance destroyed");
	pthread_mutex_destroy(&mLock); //使用完后删除
	pthread_cond_destroy(&mCondition);
}

bool PicPreviewController::start(char* spriteFilePath) {
	LOGI("Creating VideoDutePlayerController thread");
	decoder = new PngPicDecoder(); //先初始化解码png的类PngPicDecoder
	decoder->openFile(spriteFilePath);// 打开要渲染的文件
	pthread_create(&_threadId, 0, threadStartCallback, this);
	return true;
}

void PicPreviewController::stop() {
	LOGI("Stopping VideoDutePlayerController Render thread");
	/*send message to render thread to stop rendering*/
	pthread_mutex_lock(&mLock);//加锁
	_msg = MSG_RENDER_LOOP_EXIT;
	/* *
	 * 函数被用来释放被阻塞在指定条件变量上的一个线程。必须在互斥锁的保护下使用相应的条件变量。否则对条件变量的解锁有可能发生在锁定条件变量之前，从而造成死锁。
	 * 如果没有线程被阻塞在条件变量上，那么调用pthread_cond_signal()将没有作用。
	 * */
	pthread_cond_signal(&mCondition);
	pthread_mutex_unlock(&mLock);//释放锁

	LOGI("we will join render thread stop");
	pthread_join(_threadId, 0);
	LOGI("VideoDutePlayerController Render thread stopped");
}

void PicPreviewController::setWindow(ANativeWindow *window) {
	/*notify render thread that window has changed*/
	pthread_mutex_lock(&mLock);
	_msg = MSG_WINDOW_SET;
	_window = window;
	pthread_cond_signal(&mCondition);
	pthread_mutex_unlock(&mLock);
}

void PicPreviewController::resetSize(int width, int height) {
	LOGI("VideoDutePlayerController::resetSize width:%d; height:%d", width, height);
	pthread_mutex_lock(&mLock);
	this->screenWidth = width;
	this->screenHeight = height;
	renderer->resetRenderSize(0, 0, width, height);
	pthread_cond_signal(&mCondition); //解除在条件变量上的阻塞p
	pthread_mutex_unlock(&mLock);
}

void* PicPreviewController::threadStartCallback(void *myself) {
	PicPreviewController *controller = (PicPreviewController*) myself;
	controller->renderLoop();
	pthread_exit(0);
}

void PicPreviewController::renderLoop() {
	bool renderingEnabled = true;
	LOGI("renderLoop()");
	while (renderingEnabled) {
		pthread_mutex_lock(&mLock);
		/*process incoming messages*/
		switch (_msg) {
		case MSG_WINDOW_SET: //窗口有变化的情况下回重新渲染
			initialize();
			break;
		case MSG_RENDER_LOOP_EXIT:
			renderingEnabled = false;
			destroy();
			break;
		default:
			break;
		}
		_msg = MSG_NONE;

		if (eglCore) {
			eglCore->makeCurrent(previewSurface); //切换到当前EGLContext
			this->drawFrame();
			/*
			 * 函数将解锁mLock，并使当前线程阻塞在mCondition参数指向的条件变量。
			 * 被阻塞的线程可以被pthread_cond_signal函数，pthread_cond_broadcast函数唤醒，也可能在被信号中断后被唤醒，
			 * 所以函数的返回并不意味着条件的值一定发生了变化，必须重新检查条件的值。
			 * pthread_cond_wait在把线程放进阻塞队列后，自动对mutex进行解锁，使得其它线程可以获得加锁的权利，
			 * 这样其它线程才能对临界资源进行访问并在适当的时候唤醒这个阻塞的进程。当pthread_cond_wait返回的时候又自动给mutex加锁。
			 * */
			pthread_cond_wait(&mCondition, &mLock);//条件变量本身是需要互斥锁进行保护的，线程在改变条件状态前必须首先锁住互斥量
			usleep(100 * 1000); //渲染之后会等待
		}

		pthread_mutex_unlock(&mLock);
	}
	LOGI("Render loop exits");

	return;
}

bool PicPreviewController::initialize() {
	eglCore = new EGLCore();
	eglCore->init();
	previewSurface = eglCore->createWindowSurface(_window);
	eglCore->makeCurrent(previewSurface);

	picPreviewTexture = new PicPreviewTexture();
	bool createTexFlag = picPreviewTexture->createTexture();
	if(!createTexFlag){
		LOGE("createTexFlag is failed...");
		destroy();
		return false;
	}

	this->updateTexImage();

	bool isRendererInitialized = renderer->init(screenWidth, screenHeight, picPreviewTexture);
	if (!isRendererInitialized) {
		LOGI("Renderer failed on initialized...");
		return false;
	}
	LOGI("Initializing context Success");
	return true;
}

void PicPreviewController::destroy() {
	LOGI("dealloc renderer ...");
	if (NULL != renderer) {
		renderer->dealloc();
		delete renderer;
		renderer = NULL;
	}
	if(eglCore){
		eglCore->releaseSurface(previewSurface);
		eglCore->release();
		eglCore = NULL;
	}
	return;
}

void PicPreviewController::updateTexImage() {
	if (decoder) {
		const RawImageData raw_image_data = decoder->getRawImageData();
		LOGI("raw_image_data Meta: width is %d height is %d size is %d colorFormat is %d", raw_image_data.width, raw_image_data.height, raw_image_data.size,
				raw_image_data.gl_color_format);
		LOGI("colorFormat is %d", GL_RGBA);
		picPreviewTexture->updateTexImage((byte*) raw_image_data.data, raw_image_data.width, raw_image_data.height);
		decoder->releaseRawImageData(&raw_image_data);
	}
}

void PicPreviewController::drawFrame() {
	renderer->render();
	if (!eglCore->swapBuffers(previewSurface)) {
		LOGE("eglSwapBuffers() returned error %d", eglGetError());
	}
}
