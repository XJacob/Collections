package com.chap1;

import com.chap1.DogType.Dog;

public class demo {

	public void run() {
		//DesignPattern: factory method
		// create the instance inside the static factory function
		Dog pipi = Dogfactory.getDog("big");
		pipi.speak();
		
		// Item2: create a builder for the many parameter constructors
		BuilderDemo test = new BuilderDemo.Builder(10,10).length(5).size(20).build();
		test.print();
		
		// demo singleton
		SingletonDemo a = SingletonDemo.getInstance();
		SingletonDemo b = SingletonDemo.getInstance();
		a.printVal();
		a.val = 20;
		b.printVal();
		
		//avoid mem leaks, in the below example
		//if the object 3 in the array doesn't be needed, the element must be set to null
		//of it won't be freed
		String A[] = {"useful","useful","No need"}; 
		A[2] = null;  // free the third object
	}
	
	public static void main(String args[]) {
		demo tmp = new demo();
		tmp.run();
	}
}
