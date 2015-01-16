using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Profiler
{
    class ProcessTimingSeries : INotifyPropertyChanged
    {
        const uint window_size = 50;

        public Queue<float> Data
        {
            get
            {
                return data;
            }
        }
        private Queue<float> data = new Queue<float>();

        public void Add(uint timingNanos)
        {
            while (data.Count >= 50)
            {
                data.Dequeue();
            }
            data.Enqueue(timingNanos/1000000f);
            OnPropertyChanged("Data");
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string name)
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (handler != null)
            {
                handler(this, new PropertyChangedEventArgs(name));
            }
        }
    }
}
