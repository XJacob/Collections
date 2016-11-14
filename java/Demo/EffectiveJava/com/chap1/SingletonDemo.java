package com.chap1;

import static java.lang.System.out;

/*
public class SingletonDemo {
	private final static SingletonDemo INSTANCE = new SingletonDemo();
	
	public int val = 0;
	SingletonDemo() {
		out.println("I'm created!!");
	}
	
	//you can return a new instance if the singleton is unwanted
	public static SingletonDemo  getInstance() { return INSTANCE; }
	public void printVal() {
		out.println(" val: " + val);
	}
}*/

//the below is better than upper. The author recommended.
enum SingletonDemo {
	INSTANCE;
	
	public int val = 0;
	SingletonDemo() {
		out.println("I'm created!!");
	}
	
	//you can return a new instance if the singleton is unwanted
	public static SingletonDemo  getInstance() { return INSTANCE; }
	public void printVal() {
		out.println(" val: " + val);
	}
}
