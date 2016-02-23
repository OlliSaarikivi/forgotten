using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DbGen.Internal
{
    class Column : ColumnGroup
    {
        public Type Type { get; set; }
        public string Name { get; set; }

        public override IEnumerator<Column> GetEnumerator()
        {
            return Enumerable.Repeat(this, 1).GetEnumerator();
        }
    }
}
