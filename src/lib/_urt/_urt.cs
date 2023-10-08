// this one is built in now:
// using string = System.String;

namespace System
{
    // every class inherits from Object which is declared here
    public class Object
    {

        // GC support *DO NOT TOUCH!*
        private Object *__gc_next;
        private Object *__gc_prev;
        private uint    __gc_size;
        private byte    __gc_color;

        public extern virtual void Finalize ();

        public extern virtual string ToString ();
        public extern virtual bool Equals (Object obj);
        public extern virtual int GetHashCode ();
    }

    // FIXME: the C# definition of System.Type is incomplete! see _urt.h for the complete definition
    //public abstract partial class Type : System.Reflection.MemberInfo, System.Reflection.IReflect
    public class Type
    {
        private uint  _kind;
        private uint  _size;
        private char *_name;
    }

    //public sealed partial class String : IComparable, IEnumerable, IConvertible
    public class String
    {
        // FIXME: constructor support
        //public extern String (char *initialBuffer, bool owned);

        public extern static String Create (char *initialBuffer, bool owned);

        public extern static String Format (String format, params object[] args);

        private char    *_str;
        private uint     _len;
        private uint     _hashcode;
        private bool     _owned;   // true -> will DEALLOCATE() _str in finalizer
    }

    //public abstract partial class Array : System.Collections.ICollection, System.Collections.IEnumerable, System.Collections.IList, System.Collections.IStructuralComparable, System.Collections.IStructuralEquatable, System.ICloneable
    public class Array
    {
        public extern static Array CreateInstance(Type elementType, int length);
    }

    public static class Console
    {
        public extern static void WriteLine(string value);
    }


    public static class GC
    {
        public extern static void _MarkBlack (Object *obj);
    }

}

namespace System.Diagnostics
{
    public static class Debug
    {
        public extern static void Assert (bool condition);
    }
}

