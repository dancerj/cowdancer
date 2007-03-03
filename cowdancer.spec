Summary: Copy-on-write directory tree utility.
Name: cowdancer
Version: 0.25
Release: 1.%{_vendor}
License: GPL
Group: System Environment/Shells
Source: http://www.netfort.gr.jp/~dancer/software/downloads/%{name}_%{version}.tar.gz
Prefix: %{_prefix}
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
Tries to make copy-on-write semantics upon hard-link copied
directory trees generated with 'cp -la'
'cow-shell' command invokes a shell session. Within that session,
under the directory cow-shell was invoked, 
cowdancer will create a new file when existing i-nodes are opened for 
write.
Useful for quick scratch workspace and experimentation.
'cowbuilder' command is a wrapper for pbuilder which allows using 
pbuilder-like interface over cowdancer environment.

%prep
%setup -q -n %{name}
%{__perl} -pi -e 's!/usr/lib!%{_libdir}!g;' Makefile cow-shell.[1c] tests/[0-9]*

%build
%{__make} LIBDIR="%{_libdir}" CFLAGS="%{?cflags:%{cflags}}%{!?cflags:$RPM_OPT_FLAGS}" %{?mflags}

%install
%{__mkdir_p} $RPM_BUILD_ROOT%{_bindir}
%{__make} install DESTDIR=$RPM_BUILD_ROOT %{?mflags_install}

%clean
test "x$RPM_BUILD_ROOT" != "x" && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%doc COPYING NEWS ChangeLog
%{_bindir}/cow-shell
%{_libdir}/%{name}
%{_mandir}/man*/*

%changelog
4 Mar 2007 
initial specfile
