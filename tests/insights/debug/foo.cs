/*
 * overloading test 1: type compatibility + conversion
 */

using System;
using System.Diagnostics;

namespace ConsoleApp10
{
    class Program
    {
        static void f1(int a)
        {
            Console.WriteLine ("f1 int called");
            Debug.Assert(a==128);
        }

        static void Main(string[] args)
        {
            byte myb = 128;
            int  myi = 128;

            f1(myb);
            f1(myi);
        }
    }
}
