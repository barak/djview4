%define release 1
%define version 4.12

Summary: DjVu viewer
Name: djview4
Version: %{version}
Release: %{release}
License: GPL
Group: Applications/Publishing
Source: djview-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-root
URL: http://djvu.sourceforge.net

# BuildRequires: qt4-devel >= 4.2
# BuildRequires: qt4-designer >= 4.2
# BuildRequires: libjpeg-devel
# BuildRequires: libtiff-devel
# BuildRequires: glibc-devel
# BuildRequires: gcc-c++
# BuildRequires: djvulibre >= 3.5.19
# BuildRequires: xdg-utils
# Requires xdg-utils


%description 
DjView4 is a viewer and browser plugin for DjVu documents,
based on the DjVuLibre-3.5 library and the Qt4 toolkit.


%prep
%setup -q

%build
%configure
make


%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install

%clean
rm -rf %{buildroot}

%files
%defattr(-, root, root)
%doc README* COPYRIGHT COPYING NEWS
%{_bindir}
%{_datadir}
%{_mandir}

%changelog
* Thu Jan 22 2015 Leon Bottou <leonb@users.sourceforge.net> 4.9-2
- no longer rely on xdg portland tools for appfiles and icons
* Sat Jan 27 2007 Leon Bottou <leonb@users.sourceforge.net> 4.0-1
- initial release.

