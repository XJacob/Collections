package com.chap3;
import static java.lang.System.out;

public class Animal {
	public int type = 0;
	
	public final static int DOG = 1;
	public final static int CAT = 2;
	public final static int TIGER = 3;
	
	Animal(int _type) {
		type = _type;
	}
	
	// !! You must override hashCode in every class that overrides equals
	//    equal objects must have equal hash codes
	@Override
	public int hashCode() {
		int result = 17;
		result = 31 * result + type; //if type is not the same, the hash code is different
		return result;
	}
	
	@Override
	public boolean equals(Object o) {
		out.println("Override equals function is called");
		//type cast below, so the type checking is must
		if (!(o instanceof Animal))
				return false;
		if (type == (((Animal)o).type))
			return true;
		else
			return false;
	}
	
	private String type2Name(int _type)
	{
		switch(_type) {
		case DOG: return "DOG";
		case CAT: return "CAT";
		case TIGER: return "TIGER";
		}
		return " ";
	}
	
	//always override tostring to output useful information
	@Override
	public String toString() {
		return String.format("(%03d) : %s",
		type, type2Name(type));
	}
}
