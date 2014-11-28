require 'spec_helper'

describe CausesController do
  before(:each) do
  end
  
  it "should return json with causes on specific region of map" do
    get :visibles, :format => :json, :topLeftX => 16, :bottomRightX => 12, :topLeftY => 20, :bottomRightY => 11, :currentZoom => 0, :maxZoom => 6
    c1 = Cause.find(2, :select => "id, title, category_id, latitude, longitude, views, updated_at")
    c1['category_name'] = c1.category.name
    c1['url'] = c1.url
    c2 = Cause.find(4, :select => "id, title, category_id, latitude, longitude, views, updated_at")
    c2['category_name'] = c2.category.name
    c2['url'] = c2.url
    resp = [c1, c2]
    response.body.should eql(resp.to_json)
  end
end
