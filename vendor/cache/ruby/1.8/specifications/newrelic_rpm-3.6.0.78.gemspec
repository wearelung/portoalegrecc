# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "newrelic_rpm"
  s.version = "3.6.0.78"

  s.required_rubygems_version = Gem::Requirement.new("> 1.3.1") if s.respond_to? :required_rubygems_version=
  s.authors = ["Jason Clark", "Sam Goldstein", "Michael Granger", "Jon Guymon", "Ben Weintraub"]
  s.cert_chain = ["-----BEGIN CERTIFICATE-----\nMIIDODCCAiCgAwIBAgIBADANBgkqhkiG9w0BAQUFADBCMREwDwYDVQQDDAhzZWN1\ncml0eTEYMBYGCgmSJomT8ixkARkWCG5ld3JlbGljMRMwEQYKCZImiZPyLGQBGRYD\nY29tMB4XDTEzMDIxMjE5MDcwN1oXDTE0MDIxMjE5MDcwN1owQjERMA8GA1UEAwwI\nc2VjdXJpdHkxGDAWBgoJkiaJk/IsZAEZFghuZXdyZWxpYzETMBEGCgmSJomT8ixk\nARkWA2NvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAJyYRvlk1XUo\n8JhWQcE/v6RmpK//JbeKvTKnmWVUKz5oTDSOg/LKEhzChpJJVSOMJHCxd4OoxkIN\npjQF5U2af1m5ONeN1j4p4MujbwNeqxsJmixGLK/BZ9xTnbpYAa6xCRN1UfEcu3O9\njjLHX3c63ghldwRBn/c2ZD6anMtDeq3C5MLiycFs9h7JXOa3cTTHLZknkYIoHMKN\nEFri5zlks50lbeaVvFRm4IMrYWRsEwzLZWaMOy68BVZe0UlBBKSMnzJfWkbdRRcm\nxqu7viu4hrrCGjUmdHKnl6tf7BY7wqQyKjj+O5DhayKmKRuQcEX8QVnsM+ayqiVU\nEtMiwNScUnsCAwEAAaM5MDcwCQYDVR0TBAIwADAdBgNVHQ4EFgQUOauaMsU0Elp6\nhiUisj4l63ZunSUwCwYDVR0PBAQDAgSwMA0GCSqGSIb3DQEBBQUAA4IBAQAuwrHh\njOjIfAQoEbGakiwHTeImqmC1EjBEWb1+U+rC2OcsSQ3+2Q0mGq2u3lAphAeLa6i5\nWXb5OdQqZY2aI7NgMxRG98/+TcIlAT8tDR0e6/+QBlBuDXP3YI5Nuhp5U4LEvghr\njEPaEo0AFPc1JpSO/zKmktU+e9VRAE+q55gLthP8fe0uZvtGUn0KgDbXJyOuGlHF\nJ93N937OcyA2rD8gR1qkr3/0/we1dwLZnL6kN4p8nGzPgXZgOHsmTdYZ2ryYowtb\nKc9+v+QxnbZYpu2IaPXOvm3T8G4O6qZvhnLh/Uien++Dj8eFBecTPoM8pVjK3ph5\nn/EwuZCcE6ghtCCM\n-----END CERTIFICATE-----\n"]
  s.date = "2013-03-19"
  s.description = "New Relic is a performance management system, developed by New Relic,\nInc (http://www.newrelic.com).  New Relic provides you with deep\ninformation about the performance of your web application as it runs\nin production. The New Relic Ruby Agent is dual-purposed as a either a\nGem or plugin, hosted on\nhttp://github.com/newrelic/rpm/\n"
  s.email = "support@newrelic.com"
  s.executables = ["mongrel_rpm", "newrelic_cmd", "newrelic"]
  s.extra_rdoc_files = ["CHANGELOG", "LICENSE", "README.md", "GUIDELINES_FOR_CONTRIBUTING.md", "newrelic.yml"]
  s.files = ["bin/mongrel_rpm", "bin/newrelic_cmd", "bin/newrelic", "CHANGELOG", "LICENSE", "README.md", "GUIDELINES_FOR_CONTRIBUTING.md", "newrelic.yml"]
  s.homepage = "http://www.github.com/newrelic/rpm"
  s.post_install_message = "\n# New Relic Ruby Agent Release Notes #\n\n## v3.6.0 ##\n\n* Sidekiq supprt\n\nThe Ruby agent now supports the Sidekiq background job framework. Traces from\nSidekiq jobs will automatically show up in the Background tasks on New Relic\nsimilar to Resque and Delayed::Job tasks.\n\n* Improved thread safety\n\nThe primary metrics data structures in the Ruby agent are now thread safe.\nThis should provide better reliability for the agent under JRuby and threaded\nscenarios such as Sidekiq or Puma.\n\n* More robust environment report\n\nThe agent's analysis of the local environment (e.g. OS, Processors, loaded\ngems) will now work in a wider variety of app environments, including\nSinatra.\n\n* Experimental Rainbows! support\n\nThe Ruby agent now automatically detects and instruments the Rainbows! web\nserver. This support is considered experimental at present, and has not been\ntested with all dispatch modes.\n\nThanks to Joseph Chen for the contribution.\n\n* Fix a potential file descriptor leak in Resque instrumentation\n\nA file descriptor leak that occurred when DontPerform exceptions were used to\nabort processing of a job has been fixed. This should allow the Resque\ninstrumentation work correctly with the resque-lonely_job gem.\n\nSee https://github.com/newrelic/rpm/blob/master/CHANGELOG for a full list of\nchanges.\n"
  s.rdoc_options = ["--line-numbers", "--inline-source", "--title", "New Relic Ruby Agent"]
  s.require_paths = ["lib"]
  s.rubygems_version = "1.8.19"
  s.summary = "New Relic Ruby Agent"

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
    else
    end
  else
  end
end
