package com.demo.DemoUnit;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.util.Enumeration;
import java.util.Properties;
import static java.lang.System.out;

public class File extends DemoFunction {
	private final String Name = "File Usage";
	
	//file path: 1. full path
	//           2. put file to the top directory of this prorject
	//private final String PropFile = "/home/jacob/temp/java/config.prop";
	private final String PropFile = "config.prop";
	private final String tmpFile = "temp.txt";

    private void ShowProp(Properties prop) {
        if (prop == null)
            return;

        Enumeration<?> e = prop.propertyNames();
        while (e.hasMoreElements()) {
        	String key = (String)e.nextElement();
            System.out.println("Key : " + key + ", Value : " + prop.getProperty(key));
        }
    }
    
    private Properties GetProp(String FileName) {
        InputStream input = null;
        Properties prop = null;
        
        try {
            input = new FileInputStream(FileName);
            prop = new Properties();
			prop.load(input);
			prop.setProperty("phone", "99999");
			input.close();
        } catch (FileNotFoundException ex) {
            System.out.println("Can't find file:" + FileName);
        } catch (IOException ex) {
            ex.printStackTrace(System.out);
        }
    	return prop;
    }
	
    private void ReadFileText(String tmpFile) {
    	try (BufferedReader br = new BufferedReader(new FileReader(tmpFile))) {
    	    String line;
    	    while ((line = br.readLine()) != null) {
    	       out.println(line);
    	    }
    	} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
    }
    
    private void WriteFileText(String tmpFile) {
    	BufferedWriter writer = null;
    	try {
    	    writer = new BufferedWriter(new FileWriter(tmpFile));
    	    writer.write("This is a test!\n");
    	    writer.write("This is a test2!\n");
    	    writer.write("This is a test3!\n");
    	}
    	catch ( IOException e){
    		e.printStackTrace();
    	}
    	finally
    	{
    	    try {
    	        if ( writer != null)
    	        writer.close();
    	    }
    	    catch ( IOException e) {
    	    	e.printStackTrace();
    	    }
    	}
    }
    
	private void DemoProperties(String File) {
		Properties prop = GetProp(File);
		if (null != prop)
			ShowProp(prop);
	}
	
	@Override
	public void Run() {
		DemoProperties(PropFile);
		WriteFileText(tmpFile);
		ReadFileText(tmpFile);
	}

	@Override
	public String getName() {
		return Name;
	}

}
