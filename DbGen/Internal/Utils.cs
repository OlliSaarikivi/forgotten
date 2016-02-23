using System;
using System.Collections.Generic;
using System.Linq;
using System.Linq.Expressions;

namespace DbGen.Internal
{
    static class Utils
    {
        public static dynamic T(string cppType)
        {
            return new Type
            {
                CppType = cppType,
            };
        }

        public static Table Table(params ColumnGroup[] columnGroups)
        {
            return new Table
            {
                ColumnGroups = columnGroups,
            };
        }

        public static IEnumerable<int> From(dynamic table)
        {
            return null;
        }

        public static string Query<T>(Expression<Func<IEnumerable<T>>> e)
        {
            return "blah";
        }
    }
}
