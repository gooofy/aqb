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

            // create object, test functionality via object ref

            myc1 o = new myc1(23);

            // DIM o AS myc1 PTR = NEW myc1(23)

            // DIM i AS INTEGER

            // i = o->retrieve()
            // 'TRACE "i="; i
            // ASSERT i = 23

            // o->store(42)
            // i = o->retrieve()
            // 'TRACE "i="; i
            // ASSERT i = 42

            // ' now, convert o to interface ptr, test functionality by calling the intf procs

            // DIM iptr AS i1 PTR = o

            // i = iptr->retrieve()
            // 'TRACE "i="; i
            // ASSERT i = 42

            // iptr->store(23)

            // i = iptr->retrieve()
            // 'TRACE "i="; i
            // ASSERT i = 23

            // ' finally, test via object ptr again

            // ASSERT o->retrieve()  = 23
        }
    }
}


