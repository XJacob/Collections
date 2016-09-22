package com.demo;

import com.demo.DemoUnit.*;

public class DemoTest {
	
	DemoFunction Units[] = {
		new Print(), new basic(), new File()
	};
	
	void test_start() {
		System.out.print("Demo Unit Test Start...\n");
		for (DemoFunction _f : Units) {
			System.out.print("Start Running " + _f.getName() + " test::\n");
			_f.Run();
			System.out.print("End Test\n\n");
		}
	}
	
	public static void main(String args[]) {
		DemoTest a = new DemoTest();
		a.test_start();
	}
}
