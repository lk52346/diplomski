using Melanchall.DryWetMidi.Common;
using Melanchall.DryWetMidi.Core;
using Melanchall.DryWetMidi.Interaction;
using Melanchall.DryWetMidi.Multimedia;
using System;
using System.Net.Sockets;
using System.Net;
using System.Text;
using Melanchall.DryWetMidi.MusicTheory;
using System.Runtime.Serialization.Formatters.Binary;
using System.Text.Json;

class Program
{
    static int pitchShift = 0;
    static int volumeShift = 0;

    static void Main()
    {
        Listen();

        foreach (var outputDevice in OutputDevice.GetAll())
        {
            Console.WriteLine(outputDevice.Name);
        }


        using (var outputDevice = OutputDevice.GetByName("Microsoft GS Wavetable Synth"))
        {
            var midiFile = MidiFile.Read("C:\\Users\\lukak\\Desktop\\Diplomski\\HotelCalifornia-novo.mid");

            using (var playback = midiFile.GetPlayback(outputDevice))
            {

                playback.Start();


                playback.NoteCallback = (rawNoteData, rawTime, rawLength, playbackTime) =>
                    new NotePlaybackData(
                        CapNumber(rawNoteData.NoteNumber, pitchShift),
                        CapNumber(rawNoteData.Velocity, volumeShift),
                        rawNoteData.OffVelocity,
                        rawNoteData.Channel);

                
                Console.ReadLine();

            }
        }
    }

    void PrintAvailableDevices()
    {
        foreach (var outputDevice in OutputDevice.GetAll())
        {
            Console.WriteLine(outputDevice.Name);
        }
    }

    static SevenBitNumber CapNumber(SevenBitNumber number, int change)
    {
        int result = number + change;
        return (SevenBitNumber)Math.Clamp(result, 0, 127);
    }

    static void Listen()
    {
        Task.Run(async () =>
        {
            using (var udpClient = new UdpClient(12345))
            {
                while (true)
                {
                    var receivedResults = await udpClient.ReceiveAsync();
                    string jsonString = Encoding.UTF8.GetString(receivedResults.Buffer);
                    Paket? paket = JsonSerializer.Deserialize<Paket>(jsonString);
                    if (paket == null) continue;
                    Console.WriteLine(jsonString);
                    Obradi(paket, receivedResults.RemoteEndPoint);
                }
            }
        });
    }

    static void Obradi(Paket paket, IPEndPoint sender)
    {
        switch (paket.Os)
        {
            case 0:
                using (var udpClient = new UdpClient(12346))
                {
                    udpClient.Send(Encoding.ASCII.GetBytes("pong"), sender);
                }
                break;
            case 1:
                volumeShift = paket.Vrijednost*6;
                break;
            case 2:
                pitchShift = paket.Vrijednost;
                break;
            default:
                break;
        }
    }

    class Paket
    {
        public int Os { get; set; }
        public int Vrijednost { get; set; }
    }
}