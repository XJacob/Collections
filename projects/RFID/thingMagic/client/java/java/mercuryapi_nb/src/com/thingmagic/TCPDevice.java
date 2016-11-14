/*
 * Get platform information from delta digital board server
 */
package com.thingmagic;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * @author jacob
 */
public class TCPDevice extends Device {

    private final SerialReader st;
    TCPDevice(SerialReader _st){
        st = _st;
    }

    public String GetDeviceFWVersion() {
        try {
            SerialReader.Message m = new SerialReader.Message();
            m.setu8(EmbeddedReaderMessage.MSG_OPCODE_DRC_GET_FW);
            byte[] t = st.cmdCustomTransfer(m);
            if (t.length > 5) {
                return Integer.toString(t[5]);
            } else
                return "NA";
        } catch (ReaderException ex) {
            Logger.getLogger(TCPDevice.class.getName()).log(Level.SEVERE, null, ex);
        }
        return "NA";
    }

    public void DeviceReboot() {
        try {
            SerialReader.Message m = new SerialReader.Message();
            m.setu8(EmbeddedReaderMessage.MSG_OPCODE_DRC_DEVICE_REBOOT);
            st.cmdCustomTransfer(m);
        } catch (ReaderException ex) {
            Logger.getLogger(TCPDevice.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public void EnDeviceUpdate() {
        try {
            SerialReader.Message m = new SerialReader.Message();
            m.setu8(EmbeddedReaderMessage.MSG_OPCODE_DRC_EN_UPDATE);
            st.cmdCustomTransfer(m);
        } catch (ReaderException ex) {
            Logger.getLogger(TCPDevice.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public void SetGPIOs(int out[]) {
        if (out.length != GPIO_OUT_NUM) {
            System.out.println("Wrong output array");
            return;
        }

        try {
            SerialReader.Message m = new SerialReader.Message();
            m.setu8(EmbeddedReaderMessage.MSG_OPCODE_DRC_SET_GPIO);
            m.setu8(out[0]);
            m.setu8(out[1]);
            m.setu8(out[2]);
            m.setu8(out[3]);
            st.cmdCustomTransferNoRes(m);
        } catch (ReaderException ex) {
            Logger.getLogger(TCPDevice.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public int[] GetGPIOs() {
        int[] in = new int[GPIO_IN_NUM];

        try {
            SerialReader.Message m = new SerialReader.Message();
            m.setu8(EmbeddedReaderMessage.MSG_OPCODE_DRC_GET_GPIO);
            byte t[] = st.cmdCustomTransfer(m);
            if (t.length > 5) {
                in[0] = t[5];
                in[1] = t[6];
            }
        } catch (ReaderException ex) {
            Logger.getLogger(TCPDevice.class.getName()).log(Level.SEVERE, null, ex);
        }
        return in;
    }

    public void ModuleHWReset() {
        try {
            SerialReader.Message m = new SerialReader.Message();
            m.setu8(EmbeddedReaderMessage.MSG_OPCODE_DRC_MODULE_RESET);
            st.cmdCustomTransferNoRes(m);
        } catch (ReaderException ex) {
            Logger.getLogger(TCPDevice.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public void SetDeviceNetwork(String ip, String mask, String gateway) {
         try {
            SerialReader.Message m = new SerialReader.Message();
            m.setu8(EmbeddedReaderMessage.MSG_OPCODE_DRC_NETWORK);

            InetAddress _ip = InetAddress.getByName(ip);
            byte[] bytes = _ip.getAddress();
            m.setu8(bytes[0]);
            m.setu8(bytes[1]);
            m.setu8(bytes[2]);
            m.setu8(bytes[3]);

            _ip = InetAddress.getByName(mask);
            bytes = _ip.getAddress();
            m.setu8(bytes[0]);
            m.setu8(bytes[1]);
            m.setu8(bytes[2]);
            m.setu8(bytes[3]);

            _ip = InetAddress.getByName(gateway);
            bytes = _ip.getAddress();
            m.setu8(bytes[0]);
            m.setu8(bytes[1]);
            m.setu8(bytes[2]);
            m.setu8(bytes[3]);

            st.cmdCustomTransfer(m);
        } catch (ReaderException ex) {
            Logger.getLogger(TCPDevice.class.getName()).log(Level.SEVERE, null, ex);
        } catch (UnknownHostException ex) {
            Logger.getLogger(TCPDevice.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public void SetLeds(int[] antleds, int pollingTime) {
        int[] tmp = new int[]{0, 0, 0, 0};
        for (int i : antleds) {
            if (i> 0 && i < 5)
                tmp[i-1] = 1;
        }

        try {
            SerialReader.Message m = new SerialReader.Message();
            m.setu8(EmbeddedReaderMessage.MSG_OPCODE_DRC_LEDS);
            m.setu8(tmp[0]);
            m.setu8(tmp[1]);
            m.setu8(tmp[2]);
            m.setu8(tmp[3]);
            m.setu16((short) pollingTime);
            st.cmdCustomTransfer(m);
        } catch (ReaderException ex) {
            Logger.getLogger(TCPDevice.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public void SetLeds(ReadPlan rp, int pollingTime) {
        // For DRC's digital board config, only support 4 leds
        int[] AntLeds =new int[] {0,0,0,0};
        if (rp instanceof SimpleReadPlan) {
            for(int i: ((SimpleReadPlan)rp).antennas) {
                if (i > 0 && i < 5)
                    AntLeds[i - 1] = 1;
            }
            SetLeds(AntLeds, 200);
        } else if (rp instanceof MultiReadPlan) {
            for (ReadPlan sp : ((MultiReadPlan) rp).plans) {
                for (int i : ((SimpleReadPlan) sp).antennas) {
                    if (i > 0 && i < 5) {
                        AntLeds[i - 1] = 1;
                    }
                }
            }
            SetLeds(AntLeds, pollingTime);
        }
    }
    public void EnableLeds(int[] antleds) {
        SetLeds(antleds, 200);
    }
    public void DisableLeds() {
        int[] antLeds =new int[] {0,0,0,0};
        SetLeds(antLeds, 0);
    }

    public void ChangeBaudRate() {
        try {
            SerialReader.Message m = new SerialReader.Message();
            m.setu8(EmbeddedReaderMessage.MSG_OPCODE_DRC_CHANGE_BAUDRATE);
            st.cmdCustomTransferNoRes(m);
        } catch (ReaderException ex) {
            Logger.getLogger(TCPDevice.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
}
