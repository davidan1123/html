#!/bin/bash

PGM=`basename $0`

if [ `id -u` == 0 ]
then
    echo -e "$PGM should not be run as root.\n"
    exit 1
fi

bad_install()
	{
	echo "Cannot find $1 in $PWD"
	echo "Are you running $PGM in the install directory?"
	exit 1
	}

if [ ! -x $PWD/dav ]
then
	bad_install "program dav"
fi

if [ ! -d $PWD/www ]
then
	bad_install "directory www"
fi

sudo chown .www-data $PWD/www
sudo chmod 775 $PWD/www

if [ ! -d media ]
then
	mkdir media media/archive media/videos media/thumbs media/stills
	sudo chown .www-data media media/archive media/videos media/thumbs media/stills
	sudo chmod 775 media media/archive media/videos media/thumbs media/stills
fi

if [ ! -h www/media ]
then
	ln -s $PWD/media www/media
fi

if [ ! -h www/archive ]
then
	ln -s $PWD/media/archive www/archive
fi

echo ""
echo "Set the port for the nginx web server."
echo "If you already have a web server configuration using the default"
echo "port 80, you should enter an alternate port for dav."
echo "Otherwise you can use the default port 80 or an alternate as you wish."
echo "The port number will be set in: /etc/nginx.sites-available/dav."
echo -n "Enter web server port: "
read resp
if [ "$resp" == "" ]
then
	PORT=80
else
	PORT=$resp
fi

echo ""
echo "For auto starting at boot, a dav start command must be in rc.local."
echo "If you don't start at boot, dav can always be started and stopped"
echo "from the web page."
echo -n "Do you want dav to be auto started at boot? (yes/no): "
read resp
if [ "$resp" == "y" ] || [ "$resp" == "yes" ]
then
	AUTOSTART=yes
else
	AUTOSTART=no
fi


HTPASSWD=www/.htpasswd
PASSWORD=""

echo ""
if [ -f $HTPASSWD ]
then
	echo "A web password is already set."
	echo -n "Do you want to change the password (yes/no)? "
	read resp
	if [ "$resp" == "y" ] || [ "$resp" == "yes" ]
	then
		SET_PASSWORD=yes
		rm -f $HTPASSWD
	else
		SET_PASSWORD=no
	fi
else
	SET_PASSWORD=yes
fi

if [ "$SET_PASSWORD" == "yes" ]
then
	echo "Enter a password for a web page login for user: $USER"
	echo "Enter a blank entry if you do not want the password login."
	echo -n "Enter password: "
	read PASSWORD
fi




echo ""
echo "Starting dav install..."

# =============== apt install needed packages ===============
#
JESSIE=8
STRETCH=9
BUSTER=10

V=`cat /etc/debian_version`
#DEB_VERSION="${V:0:1}"
# Strip all chars after decimal point
DEB_VERSION="${V%.*}"

PACKAGE_LIST=""

if ((DEB_VERSION >= BUSTER))
then
	AV_PACKAGES="ffmpeg"
	PHP_PACKAGES="php7.3 php7.3-common php7.3-fpm"
elif ((DEB_VERSION >= STRETCH))
then
	AV_PACKAGES="libav-tools"
	PHP_PACKAGES="php7.0 php7.0-common php7.0-fpm"
else
	AV_PACKAGES="libav-tools"
	PHP_PACKAGES="php5 php5-common php5-fpm"
fi

for PACKAGE in $PHP_PACKAGES $AV_PACKAGES
do
	if ! dpkg -s $PACKAGE 2>/dev/null | grep Status | grep -q installed
	then
		PACKAGE_LIST="$PACKAGE_LIST $PACKAGE"
	fi
done

for PACKAGE in gpac nginx bc \
	sshpass mpack imagemagick apache2-utils libasound2 libasound2-dev \
	libmp3lame0 libmp3lame-dev
do
	if ! dpkg -s $PACKAGE 2>/dev/null | grep Status | grep -q installed
	then
		PACKAGE_LIST="$PACKAGE_LIST $PACKAGE"
	fi
done

if [ "$PACKAGE_LIST" != "" ]
then
	echo "Installing packages: $PACKAGE_LIST"
	echo "Running: apt-get update"
	sudo apt-get update
	sudo apt-get install -y --no-install-recommends $PACKAGE_LIST
else
	echo "No packages need to be installed."
fi


if ((DEB_VERSION < JESSIE))
then
	if ! dpkg -s realpath 2>/dev/null | grep Status | grep -q installed
	then
		echo "Installing package: realpath"
		sudo apt-get install -y --no-install-recommends realpath
	fi
fi


if [ ! -h /usr/local/bin/dav ]
then
    echo "Making /usr/local/bin/dav link."
	sudo rm -f /usr/local/bin/dav
    sudo ln -s $PWD/dav /usr/local/bin/dav
else
    CURRENT_BIN=`realpath /usr/local/bin/dav`
    if [ "$CURRENT_BIN" != "$PWD/dav" ]
    then
    echo "Replacing /usr/local/bin/dav link"
        sudo rm /usr/local/bin/dav
        sudo ln -s $PWD/dav /usr/local/bin/dav
    fi
fi


# =============== create initial ~/.dav configs ===============
#
./dav -quit

if [ "$USER" == "pi" ]
then
	rm -f www/user.php
else
	printf "<?php
    \$e_user = "$USER";
?>
" > www/user.php
fi

# =============== set install_dir in dav.conf ===============
#
dav_CONF=$HOME/.dav/dav.conf
if [ ! -f $dav_CONF ]
then
	echo "Unexpected failure to create config file $HOME/.dav/dav.conf"
	exit 1
fi

if ! grep -q "install_dir $PWD" $dav_CONF
then
	echo "Setting install_dir config line in $dav_CONF:"
	echo "install_dir $PWD"
	sed -i  "/install_dir/c\install_dir $PWD" $dav_CONF
fi


# =============== dav autostart to rc.local  ===============
#
#CMD="su $USER -c '(sleep 5; \/home\/pi\/dav\/dav)  \&'"
CMD="su $USER -c '(sleep 5; $PWD/dav) \&'"

if [ "$AUTOSTART" == "yes" ]
then
    if ! fgrep -q "$CMD" /etc/rc.local
    then
		if grep -q dav /etc/rc.local
		then
			sudo sed -i "/dav/d" /etc/rc.local
		fi
		echo "Adding a dav autostart command to /etc/rc.local:"
        sudo sed -i "s|^exit.*|$CMD\n&|" /etc/rc.local
		if ! [ -x /etc/rc.local ]
		then
			echo "Added execute permission to /etc/rc.local"
			sudo chmod a+x /etc/rc.local
		fi
		grep dav /etc/rc.local
    fi
else
	if grep -q dav /etc/rc.local
	then
		echo "Removing dav autostart line from /etc/rc.local."
		sudo sed -i "/dav/d" /etc/rc.local
	fi
fi


# ===== sudoers permission for www-data to run dav as pi ======
#
CMD=$PWD/dav
if ! grep -q "$CMD" /etc/sudoers.d/dav 2>/dev/null
then
	echo "Adding to /etc/sudoers.d: www-data permission to run dav as user pi:"
	cp etc/dav.sudoers /tmp/dav.sudoers.tmp
	sed -i "s|dav|$CMD|" /tmp/dav.sudoers.tmp
	sed -i "s/USER/$USER/" /tmp/dav.sudoers.tmp
	sudo chown root.root /tmp/dav.sudoers.tmp
	sudo chmod 440 /tmp/dav.sudoers.tmp
	sudo mv /tmp/dav.sudoers.tmp /etc/sudoers.d/dav
#	sudo cat /etc/sudoers.d/dav
fi

# =============== Setup Password  ===============
#
OLD_SESSION_PATH=www/session
if [ -d $OLD_SESSION_PATH ]
then
	sudo rm -rf $OLD_SESSION_PATH
fi

OLD_PASSWORD=www/password.php
if [ -f $OLD_PASSWORD ]
then
	rm -f $OLD_PASSWORD
fi

if [ "$PASSWORD" != "" ]
then
	htpasswd -bc $HTPASSWD $USER $PASSWORD
	sudo chown $USER.www-data $HTPASSWD
fi


# =============== nginx install ===============
#
# Logging can eat many tens of megabytes of SD card space per day
# with the mjpeg.jpg streaming
#
if ! grep -q "access_log off" /etc/nginx/nginx.conf
then
	echo "Turning off nginx access_log."
	sudo sed -i  '/access_log/c\	access_log off;' /etc/nginx/nginx.conf
fi

if ((DEB_VERSION < JESSIE))
then
	NGINX_SITE=etc/nginx-wheezy-site-default
else
	NGINX_SITE=etc/nginx-jessie-site-default
fi

echo "Installing /etc/nginx/sites-available/dav"
echo "    nginx web server port: $PORT"
echo "    nginx web server root: $PWD/www"
sudo cp $NGINX_SITE /etc/nginx/sites-available/dav
sudo sed -i "s|dav_WWW|$PWD/www|; \
			s/PORT/$PORT/" \
			/etc/nginx/sites-available/dav

if ((DEB_VERSION >= BUSTER))
then
	sudo sed -i "s/php5/php\/php7.3/" /etc/nginx/sites-available/dav
elif ((DEB_VERSION >= STRETCH))
then
	sudo sed -i "s/php5/php\/php7.0/" /etc/nginx/sites-available/dav
fi

NGINX_SITE=/etc/nginx/sites-available/dav

if [ "$PORT" == "80" ]
then
	NGINX_LINK=/etc/nginx/sites-enabled/default
	CURRENT_SITE=`realpath $NGINX_LINK`
	if [ "$CURRENT_SITE" != "$NGINX_SITE" ]
	then
		echo "Changing $NGINX_LINK link to dav"
		sudo rm -f $NGINX_LINK
		sudo ln -s $NGINX_SITE $NGINX_LINK
	fi
else
	NGINX_LINK=/etc/nginx/sites-enabled/dav
fi

if [ ! -h $NGINX_LINK 2>/dev/null ]
then
	echo "Adding $NGINX_LINK link to sites-available/dav."
	sudo ln -s $NGINX_SITE $NGINX_LINK
fi

if [ ! -f $HTPASSWD ]
then
	echo "A password for the web page is not set."
	sudo sed -i 's/auth_basic/\# auth_basic/' $NGINX_SITE
fi

sudo service nginx restart


# =============== Setup FIFO  ===============
#
fifo=$PWD/www/FIFO

if [ ! -p "$fifo" ]
then
	rm -f $fifo
	mkfifo $fifo
fi
sudo chown $USER.www-data $fifo
sudo chmod 664 $fifo



# =============== copy scripts-dist into scripts  ===============
#
if [ ! -d scripts ]
then
	mkdir scripts
fi

cd scripts-dist

for script in *
do
	if [ ! -f ../scripts/$script ] && [ "${script:0:1}" != "_" ]
	then
		cp $script ../scripts 
	fi
done

echo ""
echo "Install finished."
echo "This install script does not automatically start dav."
echo "To start dav, open a browser page to:"
if [ "$PORT" == "80" ]
then
	echo "    http://your_pi"
else
	echo "    http://your_pi:$PORT"
fi
echo "and click on the \"System\" panel and then the \"Start dav\" button."
echo "dav can also be run from a Pi terminal for testing purposes."
if [ "$AUTOSTART" == "yes" ]
then
	echo "Automatic dav starting at boot is enabled."
fi
echo ""