{
    "targets": [
        {
            "target_name": "DepthCamera",
            "sources": [ "NativeExtension.cc"],
            "include_dirs" : [
 	 			"<!(node -e \"require('nan')\")"
			]
        }
    ],
}