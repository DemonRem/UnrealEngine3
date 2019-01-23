using System.ComponentModel;
using System.Collections.ObjectModel;
using System.Windows.Media.Imaging;
using System.Windows;

namespace WPF_Landscape
{
    public class LandscapeTarget : INotifyPropertyChanged
    {
        private string layername;
        private string layerdescription;
        private BitmapSource image;
        private bool noblending;
        private float hardness;
        private bool viewmodeR, viewmodeG, viewmodeB, viewmodeNone; // Debug view mode
        private int index;

        public event PropertyChangedEventHandler PropertyChanged;

        private void NotifyPropertyChanged(string info)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(info));
            }
        }

        private void ChangeDebugColor(int debugcolor)
        {
            int R = debugcolor & 1;
            int G = debugcolor & 2;
            int B = debugcolor & 4;

            if (R > 0)
            {
                this.viewmodeR = true;
            }
            else
            {
                this.viewmodeR = false;
            }
            if (G > 0)
            {
                this.viewmodeG = true;
            }
            else
            {
                this.viewmodeG = false;
            }
            if (B > 0)
            {
                this.viewmodeB = true;
            }
            else
            {
                this.viewmodeB = false;
            }
            this.viewmodeNone = !(this.viewmodeR || this.viewmodeG || this.viewmodeB);
        }

        public LandscapeTarget()
        {
            this.noblending = false;
            this.hardness = 0.5f;
            this.viewmodeR = this.viewmodeG = this.viewmodeB = false;
            this.ViewmodeNone = true;
        }

        public LandscapeTarget(string layername, string layerdescription, string imageurl, float hardness, bool noblending, int debugcolor, int index)
        {
            this.layername = layername;
            this.layerdescription = layerdescription;
            this.image = new BitmapImage(new System.Uri(imageurl));
            this.noblending = noblending;
            this.hardness = hardness;
            this.index = index;
            ChangeDebugColor(debugcolor);
        }

        public LandscapeTarget(string layername, string layerdescription, BitmapSource image, float hardness, bool noblending, int debugcolor, int index)
        {
            this.layername = layername;
            this.layerdescription = layerdescription;
            this.image = image;
            this.noblending = noblending;
            this.hardness = hardness;
            this.index = index;
            ChangeDebugColor(debugcolor);
       }

        public string LayerName
        {
            get { return layername; }
            set { layername = value; }
        }

        public string LayerDescription
        {
            get { return layerdescription; }
            set { layerdescription = value; }
        }

        public BitmapSource Image
        {
            get { return image; }
            set { image = value; }
        }

        public float Hardness
        {
            get { return hardness; }
            set 
            {
                if (value != this.hardness)
                {
                    this.hardness = value;
                    NotifyPropertyChanged("Hardness");
                }
            }
        }

        public bool ViewmodeR
        {
            get { return viewmodeR; }
            set
            {
                if (value != this.viewmodeR)
                {
                    this.viewmodeR = value;
                    if (this.ViewmodeR)
                    {
                        this.viewmodeNone = false;
                    }
                    NotifyPropertyChanged("Viewmode");
                }
            }
        }

        public bool ViewmodeG
        {
            get { return viewmodeG; }
            set
            {
                if (value != this.viewmodeG)
                {
                    this.viewmodeG = value;
                    if (this.ViewmodeG)
                    {
                        this.viewmodeNone = false;
                    }
                    NotifyPropertyChanged("Viewmode");
                }
            }
        }

        public bool ViewmodeB
        {
            get { return viewmodeB; }
            set
            {
                if (value != this.viewmodeB)
                {
                    this.viewmodeB = value;
                    if (this.ViewmodeB)
                    {
                        this.viewmodeNone = false;
                    }
                    NotifyPropertyChanged("Viewmode");
                }
            }
        }

        public bool ViewmodeNone
        {
            get { return viewmodeNone; }
            set
            {
                if (value != this.viewmodeNone)
                {
                    this.viewmodeNone = value;
                    if (this.viewmodeNone)
                    {
                        this.viewmodeR = this.viewmodeG = this.viewmodeB = false;
                    }
                    NotifyPropertyChanged("Viewmode");
                }
            }
        }

        public bool NoBlending
        {
            get { return noblending; }
            set { noblending = value; }
        }

        public int Index
        {
            get { return index; }
            set { index = value; }
        }
    }

    public class LandscapeTargets : ObservableCollection<LandscapeTarget>
    {
    }

    // Landscape import layer listbox
    public class LandscapeImportLayer : INotifyPropertyChanged
    {
        private string layerfilename;
        private string layername;
        private bool noblending;
        private float hardness;

        public event PropertyChangedEventHandler PropertyChanged;

        private void NotifyPropertyChanged(string info)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(info));
            }
        }

        public LandscapeImportLayer()
        {
            this.noblending = false;
            this.hardness = 0.5f;
        }

        public LandscapeImportLayer(string layerfilename, string layername)
        {
            this.layerfilename = layerfilename;
            this.layername = layername;
            this.noblending = false;
            this.hardness = 0.5f;
        }

        public string LayerName
        {
            get { return layername; }
            set
            {
                if (value != this.layername)
                {
                    this.layername = value;
                    NotifyPropertyChanged("LayerName");
                }
            }
        }

        public string LayerFilename
        {
            get { return layerfilename; }
            set
            {
                if (value != this.layerfilename)
                {
                    this.layerfilename = value;
                    NotifyPropertyChanged("LayerFilename");
                }
            }
        }

        public bool NoBlending
        {
            get { return noblending; }
            set
            {
                if (value != this.noblending)
                {
                    this.noblending = value;
                    NotifyPropertyChanged("NoBlending");
                }
            }
        }

        public float Hardness
        {
            get { return hardness; }
            set
            {
                if (value != this.hardness)
                {
                    this.hardness = value;
                    NotifyPropertyChanged("Hardness");
                }
            }
        }

    }

    public class LandscapeImportLayers : ObservableCollection<LandscapeImportLayer>
    {
        public LandscapeImportLayers()
            : base()
        {
            LandscapeImportLayer L = new LandscapeImportLayer("", "");
            L.PropertyChanged += ItemPropertyChanged;
            Add(L);
        }

        public void CheckNeedNewEntry()
        {
            if (Count == 0 || this[Count - 1].LayerFilename != "" || this[Count - 1].LayerName != "")
            {
                LandscapeImportLayer L = new LandscapeImportLayer("", "");
                L.PropertyChanged += ItemPropertyChanged;
                Add(L);
            }
        }

        void ItemPropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            CheckNeedNewEntry();
        }
    }
}
