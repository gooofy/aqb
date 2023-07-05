//using System;
using string = System.String;

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
         Console.WriteLine("Hello World!");
      }
   }
}
