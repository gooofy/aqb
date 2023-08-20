// this one is built in now:
// using string = System.String;

namespace System
{
    // every class inherits from Object which is declared here
    public class Object
    {

        // GC support *DO NOT TOUCH!*
        private Object __gc_next, __gc_prev;
        private uint   __gc_size;
        private byte   __gc_color;

        public extern virtual void Finalize ();

        public extern virtual string ToString ();
        public extern virtual bool Equals (Object obj);
        public extern virtual int GetHashCode ();
    }

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


