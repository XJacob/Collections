/*
 */
package reader;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.util.Enumeration;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author jacob
 */
public class Demo {
    public static final int VERSION = 1;
    public static final String PROP_FILE = "./config.prop";
    public static final int DEBUG = 1;

    public static final String KEY_TESTPERIOD = "sleep";

    private Properties prop;

    private int SleepTime;
    private ReaderDemo _demo;

    private boolean NeedRestart = true;

    class RestartCallBack implements Callback {
        @Override
        public void CallbackFunc(int msg) {
            if (1 == msg) {
                NeedRestart = true;
                System.out.println("Exception happen!! NeedRestart!!");
            }
        }
    }

    private void ShowProp() {
        if (prop == null)
            return;

        Enumeration<?> e = prop.propertyNames();
        while (e.hasMoreElements()) {
            String key = (String) e.nextElement();
            String value = prop.getProperty(key);
            System.out.println("Key : " + key + ", Value : " + value);
        }
    }

    private void init_env() {
        System.out.print("Reader Version:" + VERSION + "\n");

        //load config
        prop = new Properties();
        InputStream input;
        try {
            input = new FileInputStream(PROP_FILE);
            prop.load(input);
            input.close();
        } catch (FileNotFoundException ex) {
            System.out.println("Can't find file:" + PROP_FILE);
            prop = null;
        } catch (IOException ex) {
            ex.printStackTrace(System.out);
        }

        if (prop != null) {
            SleepTime = Integer.valueOf(prop.getProperty(KEY_TESTPERIOD, "5000"));
        } else {
            SleepTime = 5000;
        }
    }

    Demo() {
        init_env();
        if (1 == DEBUG) {
            ShowProp();
        }
    }
    private Callback _callback;
    //create a ReaderDemo class and run it
    private void run() {
        while (true == NeedRestart) {
            NeedRestart = false;

            _demo = new ReaderDemo(prop);
            _callback = new RestartCallBack();
            _demo.addRestartCallBack(_callback);
            _demo.initPlan();
            _demo.startBackgroundRun();
            try {
                if (SleepTime < 0) {
                    System.out.print("Infinite Loop start...\n");
                    for (;;) {
                        Thread.sleep(1000);
                        if (true == NeedRestart) {
                            break;
                        }
                    }
                } else {
                    System.out.print("Sleeping for " + SleepTime + " seconds...\n");
                    while( SleepTime > 0) {
                        SleepTime -= 1;
                        Thread.sleep(1000);
                        if (true == NeedRestart) {
                            break;
                        }
                    }
                }
            } catch (InterruptedException ex) {
                Logger.getLogger(Demo.class.getName()).log(Level.SEVERE, null, ex);
            }
            if (true == NeedRestart) {
                stop();
                clean();
            }
        }
    }

    //stop ReaderDemo and deltet it
    private void stop() {
        _demo.stop();
      //  _demo.WriteEPC();    //demo how to write epc data to tag
    }

    private void clean() {
        _demo.clean();
    }

    private void ModuleReset() {
        _demo.StopThread();
        _demo.ModuleReset();
    }

    private static Demo DemoModuleReset(Demo app) {
        //To demo module reset function
        try {
            Thread.sleep(1000);
            app.ModuleReset();
            Thread.sleep(5000);
        } catch (InterruptedException ex) {
            Logger.getLogger(Demo.class.getName()).log(Level.SEVERE, null, ex);
        }
        app = new Demo();
        app.run();
        return app;
    }

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        Demo app = new Demo();

        app.run();

        // To demo how to do module reset
        //app = DemoModuleReset(app);

        app.stop();
        app.clean();
        System.out.println("Demo programe end!\n");
    }

}
