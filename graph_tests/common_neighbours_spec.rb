# edges exists

require 'redis'

redis = Redis.new

describe 'Common neighbours' do

  it 'should correctly find the common neighbours between two nodes' do
    redis.flushdb
    redis.gnode 'graph1', 'a', 'b', 'c', 'd', 'e', 'f'
    redis.gedge 'graph1', 'a', 'c', 1
    redis.gedge 'graph1', 'b', 'c', 1
    redis.gedge 'graph1', 'a', 'd', 1
    redis.gedge 'graph1', 'b', 'd', 1
    redis.gedge 'graph1', 'b', 'e', 1

    common_nodes = redis.gcommon('graph1', 'a', 'b')
    common_nodes.length.should eq 2
    common_nodes.include?('c').should eq true
    common_nodes.include?('d').should eq true
    common_nodes.include?('e').should eq false

    common_nodes = redis.gcommon('graph1', 'a', 'f')
    common_nodes.empty?.should eq true
  end

end

