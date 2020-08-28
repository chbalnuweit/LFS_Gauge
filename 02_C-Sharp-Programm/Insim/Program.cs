using System;
using System.IO;
using System.IO.Ports;
using InSimDotNet.Out;
using System.Text.RegularExpressions;


class Program
{
    static void Main()
    {
        int port = 4000;              
        string host = "127.0.0.1";

        using (OutGauge outgauge = new OutGauge())
        {
            outgauge.PacketReceived += new EventHandler<OutGaugeEventArgs>(myFunction);
            outgauge.Connect(host, port);
            Console.ReadKey(true);
        }
    }

    //Funktionen
    static void myFunction(object sender, OutGaugeEventArgs e)
    {  
        //Berechnung
        double speed = e.Speed*3.6/0.89;              
        double rpm   = e.RPM/29.57;
        double temp = e.EngTemp; //noch nicht implementiert
        double fuel = 60*e.Fuel+30;
        int gear = e.Gear;

        //Datenaufbereitung
        string s_speed = string.Format("{0:000}",speed);
        string s_rpm = string.Format("{0:000}",rpm);
        string s_temp = string.Format("{0:000}", temp);
        string s_fuel = string.Format("{0:000}", fuel);
        string s_gear = gear.ToString();

        string lights_on = e.ShowLights.ToString();
        string DL_HANDBRAKE = "0";
        string DL_SIGNAL_L = "1";
        string DL_SIGNAL_R = "1";
        string DL_ABS = "0";

        if (lights_on.Contains("DL_HANDBRAKE")) { DL_HANDBRAKE = "1"; }
        if (lights_on.Contains("DL_SIGNAL_L")) { DL_SIGNAL_L = "0"; }
        if (lights_on.Contains("DL_SIGNAL_R")) { DL_SIGNAL_R = "0"; }
        if (lights_on.Contains("DL_ABS")) { DL_ABS = "1"; }  
        if (lights_on.Contains("DL_SIGNAL_ANY")) 
        { 
            DL_SIGNAL_R = "0";
            DL_SIGNAL_L = "0";
        }

        //Übergabe
        SerialPort com = new SerialPort("COM5");
        com.Open();
        com.WriteLine(s_rpm + s_speed + s_temp + s_fuel + DL_HANDBRAKE + DL_SIGNAL_R + DL_SIGNAL_L + DL_ABS + s_gear);
        com.Close();
        Console.WriteLine(s_rpm + s_speed + s_temp + s_fuel + DL_HANDBRAKE + DL_SIGNAL_R + DL_SIGNAL_L + DL_ABS + s_gear);
       
    }
}
   