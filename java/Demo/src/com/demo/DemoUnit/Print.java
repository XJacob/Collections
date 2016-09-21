package com.demo.DemoUnit;

/**
 *  This class to demo console print function and String function.
 *
 *	Important note:
 *    1. performance: StringBuilder > StringBuffer >>>> String.
 *    2. StringBuffer is threadsafe, StringBuilder is not.
 * 
 */
public class Print extends DemoFunction {
	private final String Name = "Print Function";
	private final int x = 5;
	private final double y = 3.5;
	private final Integer z = new Integer(5);
	private final char a[] = {'1','2','3','4'};
	private String s = "This is a string";
	private StringBuilder sBuilder = new StringBuilder("performance : builder > buffer >>>> string");
	private StringBuffer sBuffer = new StringBuffer("Buffer is thread safe, builder is not!");
	
	@Override
	public void Run() {
		System.out.print("Pure text output.\n");
		System.out.println("Pure text output.");  // Do not need added change new line symbol
		System.out.print("Output with variable x= " + x + " y= " + y + " z= " + z + ".\n");
		System.out.println("a = " + (new String(a)));
		System.out.println(a);
		System.out.printf("Output with variable x=%d y=%f z=%d\n", x, y, z);
		
		System.out.println("String len: " + s.length() + " campare: " + "this is a string".compareToIgnoreCase(s));
		System.out.println(s.toLowerCase() + Integer.parseInt(new String(a)));
		
		//Caution!!! The string will create a new instance every time the content changed!
		//So it's very bad to change string content on performance
		s += "123";
		sBuilder.append("123");
		sBuffer.append("123");
		System.out.println(sBuilder.toString());
		System.out.println(sBuffer.toString());
	}

	@Override
	public String getName() {
		return Name;
	}

}
