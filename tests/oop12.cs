using System;
using System.Diagnostics;

//
// OOP Test 12
//
// interfaces
//

namespace Testcase12
{
    interface i1
    {
        void store    (int i);
        int  retrieve ();
    }

    class myc1 : i1
    {
        private int field1;

        public myc1 (int initValue)
        {
            field1 = initValue;
        }

        public void store (int i)
        {
            field1 = i;
        }
        public int retrieve ()
        {
            return field1;
        }

        public int square()
        {
            return field1 * field1;
        }
    };

    class Program
    {
        static void Main(string[] args)
        {
            int i;

            // create object, test functionality via object ref

            myc1 o = new myc1(23);

            i = o.retrieve();
            //Console.Write ("i=");
            //Console.Write (i);
            //Console.WriteLine("");
            Debug.Assert(i==23);

            o.store(42);
            i = o.retrieve();
            //Console.Write ("i=");
            //Console.Write (i);
            //Console.WriteLine("");
            Debug.Assert(i==42);

            // now, convert o to an interface typed reference, test functionality by calling interface methods

            i1 myi1 = o;

            i = myi1.retrieve();
            //Console.Write ("i=");
            //Console.Write (i);
            //Console.WriteLine("");
            Debug.Assert(i==42);

            myi1.store(23);

            i = myi1.retrieve();
            //Console.Write ("i=");
            //Console.Write (i);
            //Console.WriteLine("");
            Debug.Assert(i==23);

            // finally, test via object ptr again

            Debug.Assert(o.retrieve()==23);
        }
    }
}


