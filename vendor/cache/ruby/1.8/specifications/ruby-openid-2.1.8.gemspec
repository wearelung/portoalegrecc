# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "ruby-openid"
  s.version = "2.1.8"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["JanRain, Inc"]
  s.autorequire = "openid"
  s.date = "2010-07-22"
  s.email = "openid@janrain.com"
  s.extra_rdoc_files = ["README", "INSTALL", "LICENSE", "UPGRADE"]
  s.files = ["README", "INSTALL", "LICENSE", "UPGRADE"]
  s.homepage = "http://github.com/openid/ruby-openid"
  s.rdoc_options = ["--main", "README"]
  s.require_paths = ["lib"]
  s.rubygems_version = "1.8.19"
  s.summary = "A library for consuming and serving OpenID identities."

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
    else
    end
  else
  end
end
