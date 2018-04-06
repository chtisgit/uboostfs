uboostfs
============================

uboostfs implements some parts of the boostfs::filesystem library. If you use only
those parts and no other boost library, it can serve as drop-in replacement.
Just use the namespace boostfs instead of boost::filesystem.


Usage
----------------------------

Construct/Modify paths:

```C++
auto win = boostfs::path("C:\Windows");
auto regedit = win / "system32" / "regedit.jpg";

// oh, how could that possibly happen?
regedit.replace_extension(".exe");

auto system32 = regedit.parent_path();

std::cout << win.string() << std::endl;
std::cout << system32.string() << std::endl;
std::cout << regedit.string() << std::endl;
```

Write a folder bomb:

```C++
auto was_here = current_path();
for(auto i = 1024; i >= 0; i--){
	const char *name = ((i%2) == 0) ? "foo" : "bar";
	create_directory(path(name));
	current_path(path(name));
}

if(nice_folderbomb){
	current_path(was_here);
	remove_all("foo");
}
```

Iterate over a directory:

```C++
std::cout << "Your /bin :" << std::endl;
for(auto it = directory_iterator("/bin");
	it != directory_iterator();
	++it){

	std::cout << "- " << (*it).path().string() << std::endl;
}
```


Contribution
----------------------------

If you want to contribute, feel free to pull-request any changes. I don't
want this code base to grow too large though.
If your contribution is substantial, consider adding your name to the
license statement in filesystem.cpp

