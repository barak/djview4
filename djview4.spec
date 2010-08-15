%define release 1
%define version 4.6

Summary: DjVu viewer
Name: djview4
Version: %{version}
Release: %{release}
License: GPL
Group: Applications/Publishing
Source: djview4-%{version}.tar.gz
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

# This spec files does not install the plugin nsdejavu.so.
# Instead it relies on the (identical) version provided by djvulibre.
rm %{buildroot}%{_libdir}/netscape/plugins/nsdejavu.so
rm %{buildroot}%{_mandir}/man1/nsdejavu.1

# Remove symlinks to djview when there are alternatives
if test -x /usr/sbin/update-alternatives ; then
  test -h %{buildroot}%{_bindir}/djview \
    && rm %{buildroot}%{_bindir}/djview
  test -h %{buildroot}%{_mandir}/man1/djview.1 \
    && rm %{buildroot}%{_mandir}/man1/djview.1
fi


%clean
rm -rf %{buildroot}


%post 
# ALTERNATIVES
if test -x /usr/sbin/update-alternatives ; then
  m1=`ls -1 %{_mandir}/man1/djview4.* | head -1`
  m2=`echo $m1 | sed -e 's/djview4/djview/'`
  /usr/sbin/update-alternatives \
    --install %{_bindir}/djview djview %{_bindir}/djview4 104 \
    --slave $m2 `basename $m2` $m1
fi
# MENU
test -x %{_datadir}/djvu/djview4/desktop/register-djview-menu &&
  %{_datadir}/djvu/djview4/desktop/register-djview-menu install 2>/dev/null
exit 0


%preun
if test "$1" = 0 ; then 
 # MENU
 test -x %{_datadir}/djvu/djview4/desktop/register-djview-menu &&
   %{_datadir}/djvu/djview4/desktop/register-djview-menu uninstall 2>/dev/null
 # ALTERNATIVES
 if test -x /usr/sbin/update-alternatives ; then
   /usr/sbin/update-alternatives --remove djview %{_bindir}/djview4
 fi
fi
exit 0


%files
%defattr(-, root, root)
%doc README* COPYRIGHT COPYING INSTALL NEWS TODO
%{_bindir}
%{_datadir}/djvu/djview4
%{_mandir}


%changelog
* Sat Jan 27 2007 Leon Bottou <leonb@users.sourceforge.net> 4.0-1
- initial release.

