package com.chap4;

import java.util.ArrayList;
import java.util.List;

public class item {
	// make object information as inaccessable as possible
	private int x;
	private int y;
	private List<Object> nameList = new ArrayList<String>();
	
	item(int _x, int _y) {
		x = _x;
		y = _y;
	}
	
	public int get_x() { return x; }
	public int get_y() { return y; }
	public void set_x(int _x) { x = _x; }
	public void set_y(int _y) { y = _y; }
	public void set_name(String x) { addItem(nameList, "test".toString()); }
	
	//Don't use raw type generics, 
	//addItem(List t, Object x)   <-- not good
	private void addItem(List<String> t ,String x) {
		t.add(x);
	}
}
