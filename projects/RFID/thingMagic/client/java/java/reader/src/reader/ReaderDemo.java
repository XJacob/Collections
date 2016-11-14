/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package reader;

import com.thingmagic.*;
import static com.thingmagic.TMConstants.*;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.lang.reflect.Array;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.List;
import java.util.Properties;
import java.util.Random;
//import java.util.logging.Level;
//import java.util.logging.Logger;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.LogManager;


/**
 *
 * @author jacob
 */
public class ReaderDemo {

    private final int KEY_RETURNLOSS = 1;
    private final int KEY_TEMP = 2;
    private final int KEY_FW_VERSION = 3;
    private final int KEY_MODEL_NAME = 4;

    public static final String DEFAULT_SERILA_PATH = "tmr:///dev/ttymxc3";
    public static final String DEFAULT_TCP_PATH = "tcp://127.0.0.1";

    public static final String KEY_ANT = "antenna";
    public static final String KEY_URI = "uri";
    public static final String KEY_REGION = "region";
    public static final String KEY_POWER = "power";
    public static final String KEY_FW = "firmware";
    public static final String KEY_MULTI = "fixedpolling";
    public static final String KEY_TID = "tid";
    public static final String KEY_HOPS = "hoptable";
    public static final String KEY_TARGET = "gen2_target";
    public static final String KEY_HOPTIME = "hoptime";
    public static final String KEY_QALGORITHM = "gen2_Q";
    public static final String KEY_ENCODING = "gen2_tagEncoding";
    public static final String KEY_SESSION = "gen2_session";
    public static final String KEY_BLF = "gen2_blf";
    public static final String KEY_TARI = "gen2_tari";
    public static final String KEY_FASTSEARCH = "fast_search";

    private final int PRINT_SPEED = 1;
    //config prop
    private Device platform;
    private String readerURI;
    private String region;
    private String target;
    private String Q_algorithm;
    private String gen2Encoding;
    private String session;
    private String blf;
    private String tari;
    private String fwName;
    private int[] antennaList;
    private int[] HopList;
    private int temperature = 0;
    private int asyncOnTime = 0;
    private int readTid = 0;
    private int power = 0;
    private int hoptime = 0;
    private boolean useFastSearch = false;


    private Callback RestartCallBack;
    private Reader r;
    private ReadExceptionListener exceptionListener;
    private ReadListener rl;
    private boolean InventoryRun;
    private List<TagReadDataItem> TagDB = new ArrayList<>();
    private ShowTagThd t1 = new ShowTagThd();
    private long TagCount;
    private long StartTime = 0;
    private FileInputStream fileStream;

    private final int SimpleAsyncOnTime = 5000;

    private static final Logger logger = LogManager.getLogger("test");

    private int[] FillArray(String[] vals) {
        int i = 0;
        int[] tmp = new int[vals.length];
        for (String val : vals) {
            tmp[i] = Integer.parseInt(val);
            i++;
        }
        return tmp;
    }

    private boolean isInteger(String string) {
        try {
            Integer.valueOf(string);
            return true;
        } catch (NumberFormatException e) {
            return false;
        }
    }

    private String unparseValue(Object o) {

        if (o == null) {
            return "null";
        }

        if (o.getClass().isArray()) {
            int l = Array.getLength(o);
            StringBuilder sb = new StringBuilder();
            sb.append("[");
            for (int i = 0; i < l; i++) {
                sb.append(unparseValue(Array.get(o, i)));
                if (i + 1 < l) {
                    sb.append(",");
                }
            }
            sb.append("]");
            return sb.toString();
        } else if (o instanceof SimpleReadPlan) {
            SimpleReadPlan s = (SimpleReadPlan) o;

            return String.format("SimpleReadPlan:[%s,%s,%s,%d]",
                    unparseValue(s.protocol),
                    unparseValue(s.antennas),
                    unparseValue(s.filter),
                    s.weight);
        } else if (o instanceof TagProtocol) {
            return String.format("TagProtocol:%s", o);
        } else if (o instanceof String) {
            return "string: " + (String) o;
        }

        return o.toString();
    }

    private void getParam(Reader r, String args[])
            throws ReaderException {
        Object val;

        if (args.length == 0) // get everything
        {
            String[] params = r.paramList();
            java.util.Arrays.sort(params);
            args = params;
        }

        for (String param : args) {
            try {
                val = r.paramGet(param);
            } catch (IllegalArgumentException ie) {
                System.err.printf("No parameter named %s\n", param);
                continue;
            }

            System.out.printf("%s: %s\n", param, unparseValue(val));
        }
    }

    void showPropValue(Reader r) throws ReaderException {
        String names[] = {
            TMR_PARAM_ANTENNA_CONNECTEDPORTLIST, TMR_PARAM_REGION_ID, TMR_PARAM_RADIO_READPOWER,
            TMR_PARAM_REGION_HOPTABLE, TMR_PARAM_GEN2_TARGET,
            TMR_PARAM_REGION_HOPTIME, TMR_PARAM_GEN2_Q, TMR_PARAM_GEN2_TAGENCODING,
            TMR_PARAM_GEN2_SESSION, TMR_PARAM_GEN2_BLF, TMR_PARAM_GEN2_TARI
        };
        getParam(r, names);
    }

    void GetPropFromConfig(Properties prop) {
        String tmp;

        readerURI = prop.getProperty(KEY_URI, DEFAULT_TCP_PATH);
        String[] antennas = prop.getProperty(KEY_ANT, "1").split(",");
        String[] hops = prop.getProperty(KEY_HOPS).split(",");

        if (antennas.length > 0)
            antennaList = FillArray(antennas);

        if (hops != null && hops.length > 0 && !hops[0].isEmpty())
            HopList = FillArray(hops);

        tmp = prop.getProperty(KEY_POWER, "3000");
        if (isInteger(tmp))
            power = Integer.valueOf(tmp);

        tmp = prop.getProperty(KEY_MULTI, "0");
        if (isInteger(tmp))
            asyncOnTime = Integer.valueOf(tmp);

        tmp = prop.getProperty(KEY_TID, "0");
        if (isInteger(tmp))
            readTid = Integer.valueOf(tmp);

        tmp = prop.getProperty(KEY_HOPTIME, "0");
        if (isInteger(tmp)) {
            hoptime = Integer.valueOf(tmp);
        }

        tmp = prop.getProperty(KEY_FASTSEARCH, "0");
        if ("1".equals(tmp)) {
            useFastSearch = true;
        }

        Q_algorithm = prop.getProperty(KEY_QALGORITHM, "");
        if (!isInteger(Q_algorithm)) {
            Q_algorithm = "";
        }

        region = prop.getProperty(KEY_REGION, "OPEN");
        fwName = prop.getProperty(KEY_FW, "");
        target = prop.getProperty(KEY_TARGET, "AB");
        gen2Encoding = prop.getProperty(KEY_ENCODING, "");
        session = prop.getProperty(KEY_SESSION, "");
        blf = prop.getProperty(KEY_BLF, "");
        tari= prop.getProperty(KEY_TARI, "");
    }

    ReaderDemo(Properties prop) {
        GetPropFromConfig(prop);

        try {
            if (readerURI.contains("tcp")) {
                Reader.setSerialTransport("tcp", new SerialTransportTCP.Factory());
            }
            r = Reader.create(readerURI);
            r.connect();

            if (!fwName.isEmpty()) {
                System.out.println("Please wait 3-10 seconds...");
                UpdateFirmware(fwName);
                Thread.sleep(2000);
            }

            platform = ((SerialReader)r).GetPlatform();
            if (platform != null) {
                System.out.println("FW version: " + GetFwVersion(platform));
            } else
                System.out.println("Can't get platform data");

            //get module info
            GetParam(KEY_MODEL_NAME);
            GetParam(KEY_FW_VERSION);
            //GetParam(KEY_RETURNLOSS);  //failed on firmware 1.19

            //int pwr[][] = new int[][] {{1,3000}, {2,2000}, {3,3000}, {4,2000}};
            //r.paramSet(TMConstants.TMR_PARAM_RADIO_PORTREADPOWERLIST, pwr);

            //set region
            if (!region.equals(r.paramGet(TMR_PARAM_REGION_ID).toString())) {
                Reader.Region[] supportedRegions = (Reader.Region[]) r.paramGet(TMConstants.TMR_PARAM_REGION_SUPPORTEDREGIONS);
                for (Reader.Region t : supportedRegions) {
                    if (region.equals(t.toString())) {
                        r.paramSet(TMR_PARAM_REGION_ID, t);
                        break;
                    }
                }
            }
            System.out.println("Region: " + r.paramGet(TMR_PARAM_REGION_ID).toString());

            if (power != 0)
                r.paramSet(TMConstants.TMR_PARAM_RADIO_READPOWER, power);
            System.out.println("Set power to: " + Integer.toString(power));

            if ("OPEN".equals(region)) {
                if (HopList == null || HopList.length == 0) {
                    /* Hardcoded frequency hopping table for taiwan */
                    int[] valueList = new int[19];
                    for (int i = 0; i < 19; i++) {
                        valueList[i] = 922750 + i * 250;
                    }
                    shuffleArray(valueList);
                    r.paramSet(TMConstants.TMR_PARAM_REGION_HOPTABLE, valueList);
                } else
                    r.paramSet(TMConstants.TMR_PARAM_REGION_HOPTABLE, HopList);

                System.out.println("Set the hopping table done!");
            }

            if (hoptime != 0)
                r.paramSet(TMR_PARAM_REGION_HOPTIME, hoptime);

            if (!Q_algorithm.isEmpty()) {
                if ("DynamicQ".equals(Q_algorithm)) {
                    r.paramSet(TMConstants.TMR_PARAM_GEN2_Q, new Gen2.DynamicQ());
                } else
                    r.paramSet(TMConstants.TMR_PARAM_GEN2_Q, new Gen2.StaticQ(Integer.valueOf(Q_algorithm)));
            }


            if (!target.isEmpty()) {
                switch (target) {
                    case "AB":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_TARGET, Gen2.Target.AB);
                        break;
                    case "BA":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_TARGET, Gen2.Target.BA);
                        break;
                    case "A":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_TARGET, Gen2.Target.A);
                        break;
                    case "B":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_TARGET, Gen2.Target.B);
                        break;
                    default:
                        break;
                }
            }

            if (!gen2Encoding.isEmpty()) {
                switch (gen2Encoding) {
                    case "FM0":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_TAGENCODING, Gen2.TagEncoding.FM0);
                        break;
                    case "M2":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_TAGENCODING, Gen2.TagEncoding.M2);
                        break;
                    case "M4":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_TAGENCODING, Gen2.TagEncoding.M4);
                        break;
                    case "M8":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_TAGENCODING, Gen2.TagEncoding.M8);
                        break;
                    default:
                        break;
                }
            }

            if (!session.isEmpty()) {
                switch (session) {
                    case "S0":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_SESSION, Gen2.Session.S0);
                        break;
                    case "S1":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_SESSION, Gen2.Session.S1);
                        break;
                    case "S2":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_SESSION, Gen2.Session.S2);
                        break;
                    case "S3":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_SESSION, Gen2.Session.S3);
                        break;
                    default:
                        break;
                }
            }

            if (!blf.isEmpty()) {
                switch (blf) {
                    case "250kHz":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_BLF, Gen2.LinkFrequency.LINK250KHZ);
                        break;
                    case "320kHz":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_BLF, Gen2.LinkFrequency.LINK320KHZ);
                        break;
                    case "640kHz":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_BLF, Gen2.LinkFrequency.LINK640KHZ);
                        break;
                    default:
                        break;
                }
            }

            if (!tari.isEmpty()) {
                switch (tari) {
                    case "25us":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_TARI, Gen2.Tari.TARI_25US);
                        break;
                    case "12.5us":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_TARI, Gen2.Tari.TARI_12_5US);
                        break;
                    case "6.25us":
                        r.paramSet(TMConstants.TMR_PARAM_GEN2_TARI, Gen2.Tari.TARI_6_25US);
                        break;
                    default:
                        break;
                }
            }
            logger.info("Reader Demo init done!");
            logger.info("Reader Demo error message test.");

            showPropValue(r);
        } catch (ReaderException re) {
            System.out.println("ReaderException: " + re.getMessage());
        } catch (FileNotFoundException | InterruptedException re) {
            System.out.println("Exception: " + re.getMessage());
        }
    }

    static void shuffleArray(int[] ar) {
        Random rnd = new Random();
        for (int i = ar.length - 1; i > 0; i--) {
            int index = rnd.nextInt(i + 1);
            // Simple swap
            int a = ar[index];
            ar[index] = ar[i];
            ar[i] = a;
        }
    }

    public void addRestartCallBack(Callback f) {
        RestartCallBack = f;
    }

    public void initPlan() {
        try {
            TagOp op = null;
            if (1 == readTid) {
                 op = new Gen2.ReadData(Gen2.Bank.TID, 0, (byte)0);
            }

            if (asyncOnTime > 0) {
                ReadPlan rp[] = new ReadPlan[antennaList.length];
                for (int i = 0; i < antennaList.length; i++)
                    rp[i] = new SimpleReadPlan(new int[]{antennaList[i]}, TagProtocol.GEN2, null, op, 1000, useFastSearch);

                MultiReadPlan testMultiReadPlan = new MultiReadPlan(rp);
                r.paramSet(TMConstants.TMR_PARAM_READ_PLAN, testMultiReadPlan);
                r.paramSet(TMConstants.TMR_PARAM_READ_ASYNCONTIME, asyncOnTime);
                System.out.println("Set async on time: " + asyncOnTime);
            } else {
                SimpleReadPlan plan = new SimpleReadPlan(antennaList, TagProtocol.GEN2, null, op, 1000, useFastSearch);
                r.paramSet(TMConstants.TMR_PARAM_READ_PLAN, plan);
                r.paramSet(TMConstants.TMR_PARAM_READ_ASYNCONTIME, SimpleAsyncOnTime);
            }

            exceptionListener = new TagReadExceptionReceiver();
            r.addReadExceptionListener(exceptionListener);
            rl = new PrintListener();
            r.addReadListener(rl);

            //Enable status callback if simple plan
            if (asyncOnTime == 0) {
                SerialReader.ReaderStatsFlag[] READER_STATISTIC_FLAGS = {SerialReader.ReaderStatsFlag.TEMPERATURE,
                    SerialReader.ReaderStatsFlag.ANTENNA};
                r.paramSet(TMConstants.TMR_PARAM_READER_STATS_ENABLE, READER_STATISTIC_FLAGS);
                StatsListener statsListener = new ReaderStatsListener();
                r.addStatsListener(statsListener);
            }

        } catch (ReaderException ex) {
            System.out.println("ReaderException: " + ex.getMessage());
        }
    }

    public void startBackgroundRun() {
        if (false == InventoryRun) {
            platform.EnableLeds(antennaList);
            r.startReading();
            StartTime = System.currentTimeMillis();
            InventoryRun = true;
            t1.start();
        } else {
            System.out.println("Can't start reading twice!");
        }

    }

    public void stop() {
        if (true == InventoryRun) {
            InventoryRun = false;
            t1.interrupt();
            r.stopReading();
            platform.DisableLeds();
            StartTime = 0;
        } else {
            System.out.println("reading didn't start!");
        }
    }

    public void StopThread() {
        platform.DisableLeds();
        InventoryRun = false;
        t1.interrupt();
    }

    public void clean() {
        if (true == InventoryRun) {
            stop();
        }
        if (null != rl) {
            r.removeReadListener(rl);
        }
        if (null != exceptionListener) {
            r.removeReadExceptionListener(exceptionListener);
        }
        r.destroy();
        r = null;
    }

    class TagReadExceptionReceiver implements ReadExceptionListener {
        String strDateFormat = "M/d/yyyy h:m:s a";
        SimpleDateFormat sdf = new SimpleDateFormat(strDateFormat);

        @Override
        public void tagReadException(com.thingmagic.Reader r, ReaderException re) {
            String format = sdf.format(Calendar.getInstance().getTime());
            System.out.println("Reader Exception: " + re.getMessage() + " Occured on :" + format);
            if (re.getMessage().equals("Connection Lost")) {
                System.exit(1);
            }

            if (null != RestartCallBack) {
                RestartCallBack.CallbackFunc(1);
            }
        }
    }

    class ShowTagThd extends Thread {
        @Override
        public void run() {
            while (InventoryRun) {
                ShowTagDB();
                SetGpios(platform);

                // read something after stop inventory....
                //r.stopReading();
                //GetGpios(platform);
                //GetParam(KEY_TEMP);
                //r.startReading();

                try {
                    Thread.sleep(PRINT_SPEED * 1000);
                } catch (InterruptedException ex) {
                }
            }
        }
    }

    class ReaderStatsListener implements StatsListener {
        public void statsRead(SerialReader.ReaderStats readerStats) {
            temperature = readerStats.temperature;
        }
    }

    private List<TagReadDataItem> cloneList(List<TagReadDataItem> TagListDB) {
        List<TagReadDataItem> clonedList = new ArrayList<>(TagListDB.size());
        synchronized (TagDB) {
            for (TagReadDataItem tmp : TagListDB) {
                clonedList.add(new TagReadDataItem(tmp.t));
            }
        }
        return clonedList;
    }

    private void ShowTagDB() {
        if (TagDB.isEmpty()) {
            System.out.println("Tag Empty!");
            return;
        }
        System.out.println("\n==================Show Tag:================");
        List<TagReadDataItem> _TagDB;
        _TagDB = (List<TagReadDataItem>) cloneList(TagDB);
        for (TagReadDataItem tmp : _TagDB) {
            System.out.println(tmp.getTagReadData().toString());
            if (1 == readTid) {
                System.out.printf("TID: ");
                for (byte b : tmp.t.getData()) {
                    System.out.printf("%02x", b);
                }
                System.out.printf("\n");
            }
        }
        long nowTime = System.currentTimeMillis();
        if (StartTime != nowTime) {
            long rate = TagCount * 1000 / (nowTime - StartTime);
            System.out.println("==count: " + TagDB.size() + " ===========rate: " + Long.toString(rate) + " tags/s" +
                    "======" + "temp: " + temperature + "=====\n");
        }
        TagCount = 0;
        StartTime = nowTime;
    }

    private boolean handleDuplicated(TagReadData tr) {
        synchronized (TagDB) {
            for (TagReadDataItem tmp : TagDB) {
                TagReadData SavedTag = tmp.t;
                if ((SavedTag.getAntenna() == tr.getAntenna())
                        && (Arrays.equals(SavedTag.getTag().epcBytes(), tr.getTag().epcBytes()))) {
                    //the same tag
                    tmp.addCount(tr);
                    return true;
                }
            }
        }
        return false;
    }

    class PrintListener implements ReadListener {
        public void tagRead(Reader r, TagReadData tr) {
            TagCount+=tr.getReadCount();
            if (!handleDuplicated(tr)) {
                TagReadDataItem tmp = new TagReadDataItem(tr);
                synchronized (TagDB) {
                    TagDB.add(tmp);
                }
            }
        }
    }

    class TagReadDataItem {
        private TagReadData t;
        private int count = 0;

        TagReadDataItem(TagReadData _t) {
            t = _t;
            count = _t.getReadCount();
        }

        public void addCount(TagReadData _t) {
            t = _t;
            count += _t.readCount;
            t.readCount = count;
        }

        public TagReadData getTagReadData() {
            return t;
        }
    }

    private void UpdateFirmware(String fwName) throws FileNotFoundException {
        System.out.println("Will update fw file: " + fwName + "\n");
        fileStream = new FileInputStream(fwName);
        System.out.println("Loading firmware...");

        try {
            r.firmwareLoad(fileStream);
            fileStream.close();
        } catch (ReaderException ex) {
            logger.error("UpdateFirmware exception", ex);
        } catch (IOException ex) {
            logger.error("UpdateFirmware exception", ex);
        }
    }

    private void GetParam(int code) {
        int temp = 0;
        String tmp;
        try {
            switch (code) {
                case KEY_RETURNLOSS:
                    int[][] returnLoss;
                    returnLoss = (int[][]) r.paramGet(TMConstants.TMR_PARAM_ANTENNA_CONNECTEDPORTLIST);
                    for (int[] rl : returnLoss) {
                        System.out.println("Antenna [" + rl[0] + "] returnloss :" + rl[1]);
                    }
                    break;
                case KEY_TEMP:
                    temp = (int) r.paramGet(TMConstants.TMR_PARAM_RADIO_TEMPERATURE);
                    System.out.print("Temp: " + Integer.toString(temp) + " C" + "\n");
                    break;
                case KEY_FW_VERSION:
                    tmp = (String) r.paramGet(TMConstants.TMR_PARAM_VERSION_SOFTWARE);
                    System.out.print("Module FW ver: " + tmp + "\n");
                    break;
                case KEY_MODEL_NAME:
                    tmp = (String) r.paramGet("/reader/version/model").toString();
                    System.out.println("Model Name: " + tmp + "\n" );
                    break;
            }
        } catch (ReaderException ex) {
            logger.error("GetParam exception", ex);
        } catch (IllegalArgumentException ex) {
            logger.error("GetParam exception", ex);
        }
    }

    public void ModuleReset() {
        ModuleReset(platform);
    }

    /*
     * below demo the platform function
     */
    private String GetFwVersion(Device p) {
        return p.GetDeviceFWVersion();
    }

    //device will reboot after 2 seconds
    private void DeviceReboot(Device p) {
        p.DeviceReboot();
    }

    //device will update after 2 seconds
    private void DeviceUpdate(Device p) {
        p.EnDeviceUpdate();
    }

    private void ModuleReset(Device p) {
        p.ModuleHWReset();
    }

    private void SetGpios(Device p) {
        int[] gpio = new int[4];
        gpio[0] = 0;
        gpio[1] = 1;
        gpio[2] = 0;
        gpio[3] = 1;
        p.SetGPIOs(gpio);
    }

    private void GetGpios(Device p) {
        int[] input = p.GetGPIOs();
        System.out.println("Input: GPIO 1-> " + input[0] + " GPIO 2-> " + input[1]);
    }

    private void SetDeviceNetwork(Device p) {
        p.SetDeviceNetwork("172.16.8.108", "255.255.255.0", "172.16.8.254");
    }

    private byte[] targetTag = {(byte) 0x01, (byte) 0x2, (byte) 0x3, (byte) 0x4, (byte) 0x5, (byte) 0x6,
                    (byte) 0x07, (byte) 0x08, (byte) 0x09, (byte) 0x10, (byte) 0x11, (byte) 0x99,};
    private byte[] NewEpcData = {(byte) 0x01, (byte) 0x2, (byte) 0x3, (byte) 0x4, (byte) 0x5, (byte) 0x6,
                    (byte) 0x07, (byte) 0x08, (byte) 0x09, (byte) 0x10, (byte) 0x11, (byte) 0x12,};

    public void WriteEPC() {
        TagFilter target = null;
        if (antennaList != null) {
            try {
                r.paramSet(TMConstants.TMR_PARAM_TAGOP_ANTENNA, antennaList[0]);
                target = new TagData(targetTag);
                Gen2.TagData epc = new Gen2.TagData(NewEpcData);

                System.out.println("write EPC to :\n");
                System.out.println(target.toString());
                Gen2.WriteTag tagop = new Gen2.WriteTag(epc);
                Object v = Enum.valueOf(Gen2.Target.class, "A");
                r.paramSet("/reader/gen2/target", v);
                r.executeTagOp(tagop, target);
            } catch (ReaderException ex) {
                logger.error("GetParam exception", ex);
            }
        }
    }
}
