## Surface、SurfaceView、SurfaceHolder及SurfaceHolder.Callback之间的关理解
### 使用delphi的内存表ClientDataSet进行理解
surface---内存表数据clientDataSet.data/clientDataSet.delta
surface就像就像内存的数据clientDataSet.data/clientDataSet.delta。


SurfaceHolder----clientDataSet
改变Surface用SurfaceHolder，改变内存表数据用clientDataSet


SurfaceHolder.Callback-----clientDataSet.回调方法
surface改变通知到SurfaceHolder.Callback，就像内存表数据发生改变会通知到clientDataSet.回调方法


SurfaceView---Gride
显示的内存的部分内容，正如用户只能看见clientDataSet.data.  
通过SurfaceView可以获取到surfaceHoder,通过gride可以获得clientDataSet



