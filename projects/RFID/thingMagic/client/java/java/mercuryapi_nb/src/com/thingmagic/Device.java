/*
 * Create this abstruct class to control digital board
 */
package com.thingmagic;

/**
 *
 * @author jacob
 */
public abstract class Device {
    public final static int GPIO_OUT_NUM=4;
    public final static int GPIO_IN_NUM=2;

    public abstract String GetDeviceFWVersion();
    public abstract void SetDeviceNetwork(String ip, String mask, String gateway);
    public abstract void EnDeviceUpdate();
    public abstract void ModuleHWReset();
    public abstract void DeviceReboot();
    public abstract void SetGPIOs(int out[]);  //4 gpio out
    public abstract int[] GetGPIOs();  //2 gpio in
    public abstract void SetLeds(int leds[], int pollingTime);
    public abstract void SetLeds(ReadPlan rp, int pollingTime);
    public abstract void EnableLeds(int[] antleds);
    public abstract void DisableLeds();
    public abstract void ChangeBaudRate();
}
