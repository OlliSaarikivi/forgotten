using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DbGen.Internal
{
    abstract class ColumnGroup : IEnumerable<Column>
    {
        public abstract IEnumerator<Column> GetEnumerator();

        IEnumerator IEnumerable.GetEnumerator()
        {
            throw new NotImplementedException();
        }
    }

    class MultiColumnGroup : ColumnGroup
    {
        private Column[] columns;

        public MultiColumnGroup(params Column[] columns)
        {
            this.columns = columns;
        }

        public override IEnumerator<Column> GetEnumerator()
        {
            return ((IEnumerable<Column>)columns).GetEnumerator();
        }
    }
}
