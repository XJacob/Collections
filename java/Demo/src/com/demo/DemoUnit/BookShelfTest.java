/**
 * 
 */
package com.demo.DemoUnit;

import static org.junit.Assert.*;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import static java.lang.System.out;

/**
 * @author jacob
 *
 */
public class BookShelfTest {

	private BookShelf shelf;
	private final int shelfSize = 10;
	
	/**
	 * @throws java.lang.Exception
	 */
	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		out.println("setUpBeforeClass");
	}

	/**
	 * @throws java.lang.Exception
	 */
	@AfterClass
	public static void tearDownAfterClass() throws Exception {
		out.println("tearDownAfterClass");
	}

	/**
	 * @throws java.lang.Exception
	 */
	@Before
	public void setUp() throws Exception {
		out.println("setUp");
		shelf = new BookShelf(shelfSize);
	}

	/**
	 * @throws java.lang.Exception
	 */
	@After
	public void tearDown() throws Exception {
		out.println("tearDown");
		shelf = null;
	}

	@Test
	public void testPutBook() {
		shelf.PutABook("note", 50);
		assertEquals(50, shelf.CheckABook("note"));
		assertEquals(shelfSize - 1, shelf.RemainSapce());
		
		for(int i=0; i<shelfSize+1; i++)
			shelf.PutABook(Integer.toString(i), 50);
		assertEquals(0, shelf.RemainSapce());
	}
	
	@Test
	public void testGetBook() {
		shelf.PutABook("note", 50);
		shelf.GetABook("note");
		assertEquals(shelfSize, shelf.RemainSapce());
		
		shelf.GetABook("note");
		assertEquals(shelfSize, shelf.RemainSapce());
	}
}
