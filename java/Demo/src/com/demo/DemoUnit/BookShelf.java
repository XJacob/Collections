package com.demo.DemoUnit;

import java.util.HashMap;
import java.util.Map;
import static java.lang.System.out;

// A book shelf class
public class BookShelf {
	private Map<String, Integer> shelf = new HashMap<>();
	private int space;
	private int shelfsize = 10;
	
	BookShelf(int size) {
		shelfsize = size;
		space = shelfsize;
	}
	
	public void PutABook(String Name, int price) {
		if (space == 0) {
			out.println("Book shelf is full");
			return;
		}
		shelf.put(Name, price);
		space--;
	}
	
	public void GetABook(String Name) {
		if (space ==  shelfsize) {
			out.println("No Book on the shelf");
			return;
		}
		shelf.remove(Name);
		space++;
	}
	
	public int CheckABook(String Name) {
		return shelf.get(Name);
	}
	
	public int RemainSapce() {
		return space;
	}
}
