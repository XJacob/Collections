package com.chap3;
import static java.lang.System.out;

public class demo {

	public void run() {
		Animal a = new Animal(Animal.CAT);
		Animal b = new Animal(Animal.CAT);
		
		//demo override equals function
		if (a == b) out.println("equals1!");
		if (a.equals(b)) out.println("equals2");
		
		out.println(a.toString() + " " +  b.toString());
		
	}
	
	public static void main(String args[]) {
		demo tmp = new demo();
		tmp.run();
	}
}