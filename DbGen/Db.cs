using System.Linq;
using System.Dynamic;
using static DbGen.Internal.Utils;

namespace DbGen
{
    class Db
    {

        static ExpandoObject CreateDbDefinition()
        {
            dynamic db = new ExpandoObject();

            dynamic int32 = T("int32");
            var bodies = Table(int32.id, T("b2Body*").body);
            db.targets = Table(int32.id, int32.target);
            
            db.targetForBody = Query<int>(() => From((object)db.bodies).Zip(From((object)db.targets), (l, r) => l));

            return db;
        }
    }
}
