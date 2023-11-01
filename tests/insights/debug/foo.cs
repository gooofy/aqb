
using System;
using System.Diagnostics;

namespace ConsoleApp10
{
    class Program
    {

        static void f1(int a)
        {
            Console.WriteLine ("f1 int called");
        }

        static void f1(string s)
        {
            Console.WriteLine ("f1 str called");
        }

        // static void Main(string[] args)
        static void Main()
        {

            f1(42);
            f1("hubba");

            //int i = 42;

            //Console.WriteLine ("hubba");

            //string s;
            //s = String.Format ("i={0}, j={1}", 42, 23);
            //int a = 10;
            //int b = 32;

            //int c = a + b;

            ////int myArray1[10];

            //System.Console.WriteLine("Hello World!");
            //Debug.Assert(c==42);

            //for (int i=0; i<3; i++)
            //{
            //    Console.WriteLine("loop");
            //    Console.Write ("i=");
            //    Console.WriteInt (i);
            //    Console.WriteLine("");
            ////    System.Console.WriteLine(String.Format ("i={0}", i));
            //}
        }
    }
}
