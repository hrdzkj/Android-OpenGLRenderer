## Surface、SurfaceView、SurfaceHolder及SurfaceHolder.Callback之间的关理解
### 使用delphi的内存表ClientDataSet进行理解
surface---内存表数据clientDataSet.data/clientDataSet.delta
surface就像就像内存的数据clientDataSet.data/clientDataSet.delta。


SurfaceHolder----clientDataSet
改变Surface用SurfaceHolder，改变内存表数据lientDataSet.data/clientDataSet.delta用clientDataSet


SurfaceHolder.Callback-----clientDataSet.onAfterXXX回调方法
surface改变通知到SurfaceHolder.Callback，就像内存表数据发生改变会通知到clientDataSet.回调方法。
surfaceCreated(SurfaceHolder holder)：当Surface第一次创建后会立即调用该函数。程序可以在该函数中做些和绘制界面相关的初始化工作，一般情况下都是在另外的线程来绘制界面，所以不要在这个函数中绘制Surface。 
surfaceChanged(SurfaceHolder holder, int format, int width,int height)：当Surface的状态（大小和格式）发生变化的时候会调用该函数，在surfaceCreated调用后该函数至少会被调用一次。 
surfaceDestroyed(SurfaceHolder holder)：当Surface被摧毁前会调用该函数，该函数被调用后就不能继续使用Surface了，一般在该函数中来清理使用的资源。


SurfaceView---Gride  
SurfaceView显示的内存surface的部分内容，正如gride只能看见clientDataSet.data.  
通过SurfaceView可以获取到surfaceHoder,通过gride可以获得clientDataSet

*********************应用场景分析**********************************
本例子的可框架图https://blog.csdn.net/ericbar/article/details/80416328
本例子的代码解释https://blog.csdn.net/a568478312/article/details/80426563

SurfaceView大概是谷歌提供给开发者最吸引人的的组件了,原因是SurfaceView的界面刷新允许在非UI线程中更新,
正因为此,很多频繁更新界面的应用,如视频播放器、游戏、动画效果总会基于SurfaceView及其子类进行开发。
在Native层,Surface对应一个ANativeWindow。

分析应用,并选择实现技术
1、处理图片运算量大,为了提高运算效率,选择使用C语言处理图片
2、需要的内存空间较大,为节约内存并提高效率,需要从C语言中读入文件,并及早释放
3. SurfaceView是View的一个特殊子类，它的目的是另外提供一个线程进行绘制操作。
它的特殊性是:在一般情况下，应用程序的View都是在主线程中onDraw()方法绘制的。
被动更新画面的场景，比如棋类，这种用view就好了。因为画面的更新是依赖于onTouch 来更新，可以直接使用 invalidate。
因为这种情况下，这一次Touch和下一次的Touch需要的时间比较长些，不会产生影响。
当需要快速,主动地更新画面的场景，或者当前渲染代码阻塞GUI线程的时间过长的时候，比如一个人在一直跑动，
这就需要一个单独的thread不停的重绘人的状态，避免阻塞main UI thread，所以显然view不合适，SurfaceView就是解决上述问题的最佳选择。

***************************java层进行绘画是这样调用的**********************
SurfaceView surfaceView = (SurfaceView) findViewById(R.id.surface);
surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {

        if (surfaceHolder == null) {
            return;
        }

        Paint paint = new Paint();
        paint.setAntiAlias(true);
        paint.setStyle(Paint.Style.STROKE);

        Bitmap bitmap = BitmapFactory.decodeFile(Environment.getExternalStorageDirectory().getPath() + File.separator + "11.jpg");  // 获取bitmap
        Canvas canvas = surfaceHolder.lockCanvas();  // 先锁定当前surfaceView的画布
        canvas.drawBitmap(bitmap, 0, 0, paint); //执行绘制操作
        surfaceHolder.unlockCanvasAndPost(canvas); // 解除锁定并显示在界面上
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {

    }
});
*******************************c++的用法***********
api;
1。线程的创建，线程通信：条件变量和互斥锁pthread_cond_XXX pthread_mutex_xxx
条件变量本身是需要互斥锁进行保护的，线程在改变条件状态前必须首先锁住互斥量

2。gl进行渲染图片的api还有待学习

