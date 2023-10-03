
using System;
using System.Diagnostics;

namespace ConsoleApp10
{
    class Program
    {
        // static void Main(string[] args)
        static void Main()
        {
            int a = 10;
            int b = 32;

            int c = a + b;

            System.Console.WriteLine("Hello World!");
            Debug.Assert(c==42);

            for (int i=0; i<3; i++)
                System.Console.WriteLine("loop");
        }
    }
}
