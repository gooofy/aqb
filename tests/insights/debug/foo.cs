//using System;

// this one is built in now:
// using string = System.String;

namespace System
{
    //public sealed partial class String : IComparable, IEnumerable, IConvertible
    public class String
    {
    }

    public static class Console
    {
        public extern static void WriteLine(string value);
    }
}

namespace ConsoleApp10
{
    class Program
    {
       // static void Main(string[] args)
       static void Main()
       {
           System.Console.WriteLine("Hello World!");
       }
    }
}
