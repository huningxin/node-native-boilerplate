{
    "targets": [
        {
            "target_name": "DepthCamera",
            "sources": [ "NativeExtension.cc"],
            "include_dirs" : [
 	 			"<!(node -e \"require('nan')\")",
 	 			"/home/nhu/devel/depthsense/include/"
			],
			"libraries": [
		        "-L/home/nhu/devel/depthsense/lib", "-lDepthSense", "-lDepthSensePlugins", "-lturbojpeg", "-lstdc++"
		    ],
        }
    ],
}