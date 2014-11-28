# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "will_paginate"
  s.version = "2.3.16"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Mislav Marohni\304\207", "PJ Hyett"]
  s.date = "2011-08-09"
  s.description = "will_paginate provides a simple API for Active Record pagination and rendering of pagination links in Rails templates."
  s.email = "mislav.marohnic@gmail.com"
  s.extra_rdoc_files = ["README.md", "LICENSE", "CHANGELOG.rdoc"]
  s.files = ["README.md", "LICENSE", "CHANGELOG.rdoc"]
  s.homepage = "https://github.com/mislav/will_paginate/wiki"
  s.rdoc_options = ["--main", "README.md", "--charset=UTF-8"]
  s.require_paths = ["lib"]
  s.rubygems_version = "1.8.19"
  s.summary = "Easy pagination for Rails"

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
    else
    end
  else
  end
end
