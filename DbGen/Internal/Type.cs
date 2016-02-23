using System.Dynamic;

namespace DbGen.Internal
{
    class Type : DynamicObject
    {
        public string CppType { get; set; }

        public override bool TryGetMember(GetMemberBinder binder, out object result)
        {
            result = new Column
            {
                Type = this,
                Name = binder.Name
            };
            return true;
        }
    }
}
