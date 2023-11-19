/*
 * overloading test 1: type compatibility + conversion
 */

using System;
using System.Diagnostics;

namespace ConsoleApp10
{
    class Program
    {
        static void f1(byte b)
        {
            //Console.WriteLine ("f1 byte called");
            Debug.Assert(b==128);
        }

        static void f1(int a)
        {
            //Console.WriteLine ("f1 int called");
            Debug.Assert(a==1280);
        }

        static void Main(string[] args)
        {
            byte   myb   = 128;
            short  myi16 = 1280;
            int    myi32 = 1280;

            f1(myb);
            f1(myi16);
            f1(myi32);
        }
    }
}
