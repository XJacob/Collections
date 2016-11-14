/*
 * Library is running on the device
 */
package com.thingmagic;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author jacob
 */
public class LocalDevice extends Device {
    public  String GetDeviceFWVersion(){
        return ReadOneLine("/sys/class/drchw/fw_ver", "NA");
    }

    public int[] GetGPIOs(){
        int[] in = new int[GPIO_IN_NUM];
        in[0] = Integer.parseInt(ReadOneLine("/sys/class/gpio/gpio18/value", "0"));
        in[1] = Integer.parseInt(ReadOneLine("/sys/class/gpio/gpio19/value", "0"));
        return in;
    }

    public void SetGPIOs(int out[]){
        if (out.length != GPIO_OUT_NUM) {
            System.out.println("Wrong output array");
            return;
        }

        WriteOneLine("/sys/class/gpio/gpio16/value", Integer.toString(out[0]), false);
        WriteOneLine("/sys/class/gpio/gpio17/value", Integer.toString(out[1]), false);
        WriteOneLine("/sys/class/gpio/gpio20/value", Integer.toString(out[2]), false);
        WriteOneLine("/sys/class/gpio/gpio21/value", Integer.toString(out[3]), false);
    }

    public void ModuleHWReset(){
        WriteOneLine("/sys/class/gpio/gpio134/value", "1", false);
        try {
            Thread.sleep(1000);
        } catch (InterruptedException ex) {
            Logger.getLogger(LocalDevice.class.getName()).log(Level.SEVERE, null, ex);
        }
        WriteOneLine("/sys/class/gpio/gpio134/value", "0", false);
    }

    public void SetDeviceNetwork(String ip, String mask, String gateway) {
        System.out.println("Will Set network config then reboot:");
        System.out.println("ip: " + ip + " mask: " + mask + " gateway: " + gateway);

        File Dirname = new File("/home/update/config");
        if (!Dirname.exists()) {
            System.out.println("creating directory: " + Dirname);
            try {
                Dirname.mkdir();
            } catch (SecurityException ex) {
                java.util.logging.Logger.getLogger(LocalDevice.class.getName()).log(Level.SEVERE, null, ex);
            }
        }

        WriteOneLine("/home/update/config/ip_addr.txt", "address:" + ip +"\n", false);
        WriteOneLine("/home/update/config/ip_addr.txt", "netmask:" + mask +"\n", true);
        WriteOneLine("/home/update/config/ip_addr.txt", "gateway:" + gateway +"\n", true);

        //The new ip will apply after rebooting
        DeviceReboot();
    }

    public void EnDeviceUpdate(){
         try {
            Process p = Runtime.getRuntime().exec("/home/root/bin/update.sh");
        } catch (IOException ex) {
            java.util.logging.Logger.getLogger(LocalDevice.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public void DeviceReboot() {
        try {
            Process p = Runtime.getRuntime().exec("reboot");
        } catch (IOException ex) {
            java.util.logging.Logger.getLogger(LocalDevice.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    private void WriteOneLine(String path, String val, boolean append) {
        File file = new File(path);
        BufferedWriter w = null;

        try {
            w = new BufferedWriter(new FileWriter(file, append));
            w.write(val);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                if (w != null) {
                    w.close();
                }
            } catch (IOException e) {
            }
        }
    }

    private String ReadOneLine(String path, String def) {
        File file = new File(path);
        BufferedReader r = null;
        String t = def;

        try {
            r = new BufferedReader(new FileReader(file));
            t = r.readLine();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                if (r != null) {
                    r.close();
                }
            } catch (IOException e) {
            }
        }
        return t;
    }

    public void SetLeds(int leds[], int pollingTime) {
        //Not support
    }
    public void SetLeds(ReadPlan rp, int pollingTime) {
        //Not support
    }
    public void EnableLeds(int[] antleds) {
        //Not support
    }
    public void DisableLeds() {
        //Not support
    }
    public void ChangeBaudRate() {
        //Not support
    }
}
