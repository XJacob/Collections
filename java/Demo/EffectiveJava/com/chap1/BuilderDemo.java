package com.chap1;

import static java.lang.System.out;

//Item2: create a builder for the many parameter constructors
public class BuilderDemo {
	int weight, height, length, size;
	
	public static class Builder {
		private int weight=0, height=0, length=0, size=0;
		
		public Builder(int weight, int height) {
			this.weight = weight;
			this.height = height;
		}
		public Builder length(int val)
		{ length = val; return this; }
		public Builder size(int val)
		{ size = val; return this; }
		
		public BuilderDemo build() {
			return new BuilderDemo(this);
		}
	}
	
	public void print() {
		out.printf("w=%d h=%d l=%d s=%d", weight, height, length, size);
	}
	
	private BuilderDemo(Builder builder) {
		weight = builder.weight;
		height = builder.height;
		length = builder.length;
		size = builder.size;
	}
}
