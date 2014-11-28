require 'active_record'
require 'active_support'
require 'nokogiri'
require 'open-uri'
require 'savon'
require 'sinatra'
require 'rest_client'
require 'ruby-debug'
require 'tzinfo'

# Initialize local timezone
Time.zone = 'Brasilia'
raw_config = File.read(File.dirname(__FILE__) + "/config.yml")
#FALAPOA_CONFIG = YAML.load(raw_config)[:falapoa]