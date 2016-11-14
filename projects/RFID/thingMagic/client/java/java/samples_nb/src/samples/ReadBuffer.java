package samples;

import com.thingmagic.Gen2;
import com.thingmagic.Reader;
import com.thingmagic.ReaderUtil;
import com.thingmagic.TMConstants;
import com.thingmagic.TagFilter;
import com.thingmagic.TagReadData;
import com.thingmagic.TransportListener;

public class ReadBuffer
{
    static BlockPermalock.SerialPrinter serialPrinter;
    static BlockPermalock.StringPrinter stringPrinter;
    static TransportListener currentListener;
    static boolean sendRawData = false;

    static void usage()
    {
        System.out.printf("Usage: Please provide valid arguments, such as:\n"
                + "  (URI: 'tmr:///COM1 --ant 1,2' or 'tmr://astra-2100d3/ --ant 1,2' "
                + "or 'tmr:///dev/ttyS0 --ant 1,2')\n\n");
        System.exit(1);
    }

    public static void setTrace(Reader r, String args[])
    {
        if (args[0].toLowerCase().equals("on"))
        {
            r.addTransportListener(Reader.simpleTransportListener);
            currentListener = Reader.simpleTransportListener;
        } else if (currentListener != null)
        {
            r.removeTransportListener(Reader.simpleTransportListener);
        }
    }

    static  int[] parseAntennaList(String[] args,int argPosition)
    {
        int[] antennaList = null;
        try
        {
            String argument = args[argPosition + 1];
            String[] antennas = argument.split(",");
            int i = 0;
            antennaList = new int[antennas.length];
            for (String ant : antennas)
            {
                antennaList[i] = Integer.parseInt(ant);
                i++;
            }
        }
        catch (IndexOutOfBoundsException ex)
        {
            System.out.println("Missing argument after " + args[argPosition]);
            usage();
        }
        catch (Exception ex)
        {
            System.out.println("Invalid argument at position " + (argPosition + 1) + ". " + ex.getMessage());
            usage();
        }
        return antennaList;
    }

    public static void main(String argv[])
    {
        Reader r = null;
        // Program setup
        TagFilter target = null;
        int nextarg = 0;
        boolean trace = false;
        int[] antennaList = null;

        if (argv.length < 1 || (argv.length == 1 && argv[nextarg].equalsIgnoreCase("-v")))
        {
            usage();
        }

        if (argv[nextarg].equals("-v"))
        {
            trace = true;
            nextarg++;
        }

        // Create Reader object, connecting to physical device
        try
        {
            TagReadData[] tagReads;
            String readerURI = argv[nextarg];
            nextarg++;

            for (; nextarg < argv.length; nextarg++)
            {
                String arg = argv[nextarg];
                if (arg.equalsIgnoreCase("--ant"))
                {
                    if (antennaList != null)
                    {
                        System.out.println("Duplicate argument: --ant specified more than once");
                        usage();
                    }
                    antennaList = parseAntennaList(argv, nextarg);
                    nextarg++;
                } else
                {
                    System.out.println("Argument " + argv[nextarg] + " is not recognised");
                    usage();
                }
            }

            r = Reader.create(readerURI);
            if (trace)
            {
                setTrace(r, new String[]
                {
                    "on"
                });
            }
            r.connect();
            
            // i got the error when reading before adding this code 405 that's why i added this code for region set
            if (Reader.Region.UNSPEC == (Reader.Region) r.paramGet("/reader/region/id")) 
            {
                Reader.Region[] supportedRegions = (Reader.Region[]) r.paramGet(TMConstants.TMR_PARAM_REGION_SUPPORTEDREGIONS);
                if (supportedRegions.length < 1) 
                {
                    throw new Exception("Reader doesn't support any regions");
                } 
                else 
                {
                    r.paramSet("/reader/region/id", supportedRegions[0]);
                }
            }

            r.paramSet("/reader/tagop/antenna", antennaList[0]);

            byte[]  key0  =  new byte[]{(byte) 0x01, (byte) 0x23, (byte) 0x45, (byte) 0x67, (byte) 0x89, (byte) 0xAB, (byte) 0xCD, (byte) 0xEF, (byte) 0x01, (byte) 0x23, (byte) 0x45, (byte) 0x67, (byte) 0x89, (byte) 0xAB, (byte) 0xCD, (byte) 0xEF};
            byte[]  key1  =  new byte[]{(byte) 0x11, (byte) 0x22, (byte) 0x33, (byte) 0x44, (byte) 0x55, (byte) 0x66, (byte) 0x77, (byte) 0x88, (byte) 0x11, (byte) 0x22, (byte) 0x33, (byte) 0x44, (byte) 0x55, (byte) 0x66, (byte) 0x77, (byte) 0x88};
            byte[] iChallenge = new byte[]{(byte) 0x01, (byte) 0x23, (byte) 0x45, (byte) 0x67, (byte) 0x89, (byte) 0xab, (byte) 0xcd, (byte) 0xef, (byte) 0xab, (byte) 0xcd};
            Gen2.NXP.AES.Tam1Authentication tam1;
            Gen2.NXP.AES.Tam2Authentication tam2;
            
            //TAM1 with key0
            tam1 = new Gen2.NXP.AES.Tam1Authentication(Gen2.NXP.AES.KeyId.KEY0,key0, iChallenge, sendRawData);
            Gen2.ReadBuffer tam1Key0 = new Gen2.NXP.AES.ReadBuffer(0, 128, tam1);
            System.out.println(tam1Key0.toString());
            byte[] response = (byte[])r.executeTagOp(tam1Key0, null);
            System.out.println("response: "+ReaderUtil.byteArrayToHexString(response));

            //TAM1 with key1
/*            tam1 = new Gen2.NXP.AES.Tam1Authentication(Gen2.NXP.AES.KeyId.KEY1,key1, iChallenge, sendRawData);
            Gen2.ReadBuffer tam1Key1 = new Gen2.NXP.AES.ReadBuffer(0, 128, tam1);
            System.out.println(tam1Key1.toString());
            response = (byte[])r.executeTagOp(tam1Key1, null);
            System.out.println("response: "+ReaderUtil.byteArrayToHexString(response));

            //TAM2 with KEY1
            tam2 = new Gen2.NXP.AES.Tam2Authentication(Gen2.NXP.AES.KeyId.KEY1,key1, iChallenge, Gen2.NXP.AES.Profile.TID, 0, 1, sendRawData);
            Gen2.ReadBuffer tam2Key1 = new Gen2.NXP.AES.ReadBuffer(0, 128, tam2);
            System.out.println(tam2Key1.toString());
            response = (byte[])r.executeTagOp(tam2Key1, null);
            System.out.println("response: "+ReaderUtil.byteArrayToHexString(response)); */
        }
        catch(Exception e)
        {
            e.printStackTrace();
        }
    }
}
