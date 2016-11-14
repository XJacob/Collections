package com.chap1;

import static java.lang.System.out;

public class DogType {
	interface Dog {
		public void speak();
	}
	
	//avoid anyone to instance this class
	private DogType (){
		throw new AssertionError();
	}
}
