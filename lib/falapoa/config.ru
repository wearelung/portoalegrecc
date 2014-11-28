# encoding: UTF-8

require 'rubygems'
require 'bundler'

Bundler.require

require './bootstrap'
require './server'
require './consumer'
require './parser'

log = File.new("log/sinatra.log", "a")
STDOUT.reopen(log)
STDERR.reopen(log)

run Sinatra::Application