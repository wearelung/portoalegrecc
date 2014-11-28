class AlterPrecisionOfLatitudeAndLongitude < ActiveRecord::Migration
  def self.up
    change_table :causes do |t|
      t.change :latitude,  :decimal, :null => false, :precision => 20, :scale => 17
      t.change :longitude, :decimal, :null => false, :precision => 20, :scale => 17
    end
  end

  def self.down
    change_table :causes do |t|
      t.change :latitude,  :decimal, :null => false
      t.change :longitude, :decimal, :null => false
    end
  end

end
