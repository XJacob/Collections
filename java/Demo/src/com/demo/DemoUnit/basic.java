package com.demo.DemoUnit;
import java.util.*;
import static java.lang.System.out;

public class basic extends DemoFunction {
	private final String Name = "Basic Usage";
	private final int[] IntArray = {1,9,3,8,4,11,7,2,23};
	
	private enum Goods {
		chair(100), desktop(200), mouse(50), keyboard(150);
		
		private int price;
		private Goods(int price) {
			this.price = price;
		}
		public int getPrice() {
			return price;
		}
		
		public String toString() {
			switch(this) {
			case mouse:
				return "My mouse";
			case keyboard:
				return "my keyboard";
			case chair:
				return "My chair";
			case desktop:
				return "Your desk";
			default:
				return null;
			}
		}
	};

	private void types() {
		//type : byte(1), short(2), int(4), long(8)
		//type :                    float(4), double(8)
		long x = 12345678899999L;
		float y = 0.3F;
		double z = 0.3;
		String name = "codedata";
		out.println( x + y + z);
		out.println(name + "(" + name.length() + ") -> " + name.contains("coded") + " " +
				name.charAt(3) + " " + name.substring(2, 5) );
	}
	
	private void arrays() {
		int[] buf = Arrays.copyOf(IntArray, IntArray.length - 1);
		
		String tmp = String.format("Array(%d): ", IntArray.length);
		out.print(tmp);
		
		for(int j: buf)
			System.out.print(j);
		out.print("\n");
		
		Arrays.sort(buf);
		for(int k = 0; k < buf.length; k++)
			System.out.print(buf[k]);
		out.print("\n");
		
		out.println(Arrays.toString(buf));
		out.println("8 is at " + Arrays.binarySearch(buf, 8));
		Arrays.fill(buf, 0);
	}
	
	private void lists() {
		List<String> strlist = new ArrayList<String>();
		List<String> initlist = Arrays.asList("abc", "def", "ghi");
		
		out.print("\nlists test: \n");
		strlist.add("test");
		strlist.addAll(initlist);
		strlist.add(3, "test1");
		//initlist.add("test1");   !! can't do it...
		
		out.printf("strlist(%d) : \n", strlist.size());
		for(String x: strlist)
			out.print(" " + x);
		out.print("\n");
		out.println("contains test: " + strlist.contains("test") + " at : " + strlist.indexOf("test"));
		out.println("get 3: " + strlist.get(3));
		
		strlist.remove("test");
		out.printf("strlist(%d) : \n", strlist.size());
		Iterator<String> iterator = strlist.iterator();
		while(iterator.hasNext())
			out.print(" " + iterator.next());
		out.print("\n");
		
		strlist.set(1, "first");
		out.println(strlist);
		out.print("\n");
		
		List<Goods> goods = new ArrayList<Goods>();
		goods.add(Goods.chair);
		goods.add(Goods.mouse);
		for (Goods i: goods) {
			out.printf("name: %s price:%d\n", i.toString(), i.getPrice());
		}
	}
	
	private void sets() {
		Set<String> GroupA = new HashSet<>(Arrays.asList("Jacob", "Bill"));
		Set<String> GroupB = new HashSet<>(Arrays.asList("Jane", "Julia", "Jacob"));
		
		out.print("\nSets test: \n");
		Iterator<String> iterator = GroupA.iterator();
		GroupA.add("Jacob"); // it's useless to add the same value twice
		GroupA.add("jacob");
		
		//if the set has been changed, need to get the iterator again
		iterator = GroupA.iterator(); 
		while (iterator.hasNext())
			out.print(" " + iterator.next());
		out.print("\n");
		
		out.println(GroupA);
		GroupA.retainAll(GroupB);
		out.print(" Name in Group A and B: ");
		out.println(GroupA);
		out.print("\n");
	}
	
	private void maps() {
		out.print("\nmaps test: \n");
		
		Map<String, Integer> passwords = new HashMap<>();
        passwords.put("Justin", 123456);
        passwords.put("caterpillar", 93933);
        passwords.put("Hamimi", 970221); 
        out.println(passwords);
        
        if(passwords.containsValue("Justin"))
        	out.println("Justin : " + passwords.get("Justin"));
        passwords.remove("caterpillar"); 
        out.println(passwords);
        out.println(passwords.keySet());
        out.println("size: " + passwords.size() + passwords.values());
	}
	
	@Override
	public void Run() {
		types();
		arrays();
		lists();
		sets();
		maps();
	}

	@Override
	public String getName() {
		return Name;
	}

}
