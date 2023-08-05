
// this one is built in now:
// using string = System.String;

namespace System
{
    //public sealed partial class String : IComparable, IEnumerable, IConvertible
    public class String
    {
        public extern static String Create (char *initialBuffer, bool owned);
    }

    public static class Console
    {
        public extern static void WriteLine(string value);
    }
}

using System;

namespace ConsoleApp10
{
    class Program
    {
       // static void Main(string[] args)
       static int Main()
       {
           System.Console.WriteLine("Hello World!");
       }
    }
}
