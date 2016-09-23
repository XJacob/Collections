package com.demo.DemoUnit;

public class ThdDemo extends DemoFunction {
	private final String Name = "Thread Usage";
	private int tmp_val = 10;
	private static Object lock = new Object();
	
	//with the synchronized, there is only one thread run this function per time
	private synchronized void  SyncFuncDemo(String name) {
		try {
			Thread.sleep(100);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
	}
	
	public class MyThd extends Thread {
		boolean bWait = false;
		private long Id = this.getId();
		private String Name = this.getName();
		
		private synchronized void IntoWait(String name) {
			System.out.println("Thread " + name + " in to wait ...");
			try {
				wait();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
			System.out.println("Thread " + name + " out of wait...");
		}

		public synchronized void OutWait() {
			notifyAll();
		}		
		
		MyThd(boolean wait, String name) {
			super(name);
			bWait = wait;
		}
		public void run() {
			System.out.println("Thread " + Name + " in...");
			if (!bWait)
				SyncFuncDemo(Name);
			else
				IntoWait(Name);
			
			// without the synchronized, the tmp_val may be added before set to 10
			// by the second thread
			synchronized (lock) {
				System.out.println("Here is the starting point of Thread " + Name + " id: " + Id);
				try {
					tmp_val += 10;
					Thread.sleep(100);
					System.out.println(Name + " : tmp val = " + tmp_val);
					tmp_val = 10;
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
			System.out.println("Thread " + Name + " out...");
		}
	}
	
	@Override
	public void Run() {
		MyThd t1 = new MyThd(false, "t1");
		MyThd t2 = new MyThd(false, "t2");
		MyThd t3 = new MyThd(true, "t3");
		// t3 will into wait state until it is be notified..
		t3.start();
		
		t1.start();
		t2.start();
		try {
			t1.join();
			t2.join();
			t3.OutWait();
			t3.join();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		
		// runnable demo below
		Thread t4 = new Thread(new Runnable() {
			private String tag = "runnable test";
			
			public void run() {
				System.out.println(tag + " : " + Thread.currentThread().getName());
				try {
					Thread.sleep(100);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		}, "t4");
		
		t4.start();
		try {
			t4.join();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		System.out.println("Thread run terminated!");
	}

	@Override
	public String getName() {
		return Name;
	}

}
