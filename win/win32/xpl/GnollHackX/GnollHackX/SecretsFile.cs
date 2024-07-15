﻿using System;
using System.Collections.Generic;
using System.Text;

namespace GnollHackX
{
    public class SecretsFile
    {
        public string id;
        public string name;
        public string type;
        public int subtype_id;
        public string description;
        public string version;
        public string source_directory;
        public long length_mobile;
        public long length_desktop;
        public string target_directory;
        public int on_demand;
        public int streaming_asset;
        public int flags;

        public SecretsFile()
        {

        }
    }
}
