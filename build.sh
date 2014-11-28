#!/bin/sh
echo "Starting Build"

bundle install --local --path vendor/bundle --binstubs
#bundle install --path vendor/bundle --binstubs
if [ "$?" -ne "0" ]; then
	echo "Failure on bundle install!"
	exit 1
fi

bundle exec rake build
if [ "$?" -ne "0" ]; then
	echo "Failure on rake build!"
	exit 1
fi
echo "Finishing  Build"
exit 0