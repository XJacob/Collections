package com.chap1;

import com.chap1.DogType.Dog;

public class Dogfactory
{
	public static Dog getDog(String criteria) {
		if (criteria.equals("small"))
			return new Poodle();
		else if (criteria.equals("big"))
			return new Husky();
		return null;
	}
}
