require 'redis'

redis = Redis.new

describe 'basic commands' do
  before do
    redis.flushdb
    redis.gnode 'graph1', 'a', 'b', 'c', 'd', 'e', 'f', 'g'
    redis.gsetdirected 'graph1'
    redis.gedge 'graph1', 'a', 'c', 1
    redis.gedge 'graph1', 'b', 'c', 1
    redis.gedge 'graph1', 'c', 'd', 1
    redis.gedge 'graph1', 'd', 'b', 1
    redis.gedge 'graph1', 'e', 'c', 1
    redis.gedge 'graph1', 'a', 'e', 1
    redis.gedge 'graph1', 'c', 'g', 1
    redis.gedge 'graph1', 'c', 'b', 1
    redis.gedge 'graph1', 'b', 'g', 1
  end

  it "should calculate the correct page rank values" do
    ret = redis.granks 'graph1'

    puts ret.inspect
  end
end
