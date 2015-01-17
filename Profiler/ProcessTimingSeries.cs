using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Profiler
{
    class ProcessDataPoint
    {
        public int Tick { get; set; }
        public float Duration { get; set; }
    }

    class ProcessTimingSeries : INotifyPropertyChanged
    {
        const int window_size = 10;

        public ObservableCollection<ProcessDataPoint> Data
        {
            get
            {
                return data;
            }
        }
        private ObservableCollection<ProcessDataPoint> data = new ObservableCollection<ProcessDataPoint>();

        public void Add(int tick, uint timingNanos)
        {
            while (data.Count >= window_size)
            {
                data.RemoveAt(0);
            }
            data.Add(new ProcessDataPoint {
                Tick = tick,
                Duration = timingNanos/1000000f,
            });
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
