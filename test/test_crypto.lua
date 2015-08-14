local crypto = require 'crypto'

local html_text = [[
<tt class="descclassname">HMAC.</tt><tt class="descname">hexdigest</tt><big>(</big><big>)</big><a class="headerlink" href="#hmac.HMAC.hexdigest" title="Permalink to this definition">?</a></dt>
<dd><p>Like <a class="reference internal" href="#hmac.HMAC.digest" title="hmac.HMAC.digest"><tt class="xref py py-meth docutils literal"><span class="pre">digest()</span></tt></a> except the digest is returned as a string twice the length
containing only hexadecimal digits.  This may be used to exchange the value
<p class="last">When comparing the output of <a class="reference internal" href="#hmac.HMAC.hexdigest" title="hmac.HMAC.hexdigest"><tt class="xref py py-meth docutils literal"><span class="pre">hexdigest()</span></tt></a> to an externally-supplied
digest during a verification routine, it is recommended to use the
<a class="reference internal" href="#hmac.compare_digest" title="hmac.compare_digest"><tt class="xref py py-func docutils literal"><span class="pre">compare_digest()</span></tt></a> function instead of the <tt class="docutils literal"><span class="pre">==</span></tt> operator
to reduce the vulnerability to timing attacks.</p>]]

local pubkey_text = "\
-----BEGIN RSA PUBLIC KEY-----\
MIGHAoGBALjUhW3l3ZP0MvIL5xzqgO63T6EMmEgCxBE2CCX+PPVk534rJcHmcwZZ\
hnsBOr+qeCnLeX/wM8oQa7mPPSn0y/cMUzfFygwwEXWLp+KuCS6edCs5epG8NvqL\
Ix4kxv9xumMql6j6oLmglEboO/j6djk7MHVlUGvcCYRZiLZTpo3BAgED\
-----END RSA PUBLIC KEY-----\
"
local prikey_text = "\
-----BEGIN RSA PRIVATE KEY-----\
MIICXAIBAAKBgQC41IVt5d2T9DLyC+cc6oDut0+hDJhIAsQRNggl/jz1ZOd+KyXB\
5nMGWYZ7ATq/qngpy3l/8DPKEGu5jz0p9Mv3DFM3xcoMMBF1i6firgkunnQrOXqR\
vDb6iyMeJMb/cbpjKpeo+qC5oJRG6Dv4+nY5OzB1ZVBr3AmEWYi2U6aNwQIBAwKB\
gHs4WPPuk7f4IfaymhNHAJ8k38CzEDAB2At5WsP+005DRP7HboFETK7mWadWJypx\
pXEyUP/1d9wK8nu003FN3U468HbNVhlLmMRS7GAWjKopANEiJsl3J96h+zzoBxIK\
3Yy9MkXK2IYcIz1fkCheXGF7sNSL5eJr2orh4H2l48sDAkEA57ZzvGMioYgcJ2iG\
P+UeeM106VmyFJu8ujAGCVaO1Bf1S6edi2PTdkWOdnvfCW/kfjI4Xi6s7BjMOw3t\
r95/TwJBAMw0EdVlw50kMufczExREOglfJzmsXTfb936PL9l1Y1WGsMkor74HQAZ\
3GJn3WN7woN0/jVI604o58uqDCryXe8CQQCaeaJ9l2xrsBLE8Fl/7hRQiPibkSFj\
En3RdVlbjwniuqOHxROyQoz5g7RO/T9bn+2pdtA+ycidZd18s/PKlFTfAkEAiCK2\
jkPXvhgh7+iIMuC18Bj9ve8g+JT1PqbTKkPjs468ghhsf1ATVWaS7EU+QlKBrPip\
eNtHiXCah8ayx0w+nwJBANC533QDDhUMQvj+yOxxvwq7dgBaOdxuILB3M4cZ4qHs\
frmAihdFxGlhB3jYjcnyNDza4LuOuQHq/JiverlqQfs=\
-----END RSA PRIVATE KEY-----\
"

local function digest(t, s, mode)
    mode = mode or 'hex'
    local hash = crypto.createHash(t)
    hash:update(s)
    local checksum = hash:digest(mode)
    hash:clear()
    return checksum
end

local function test_md5()
    local checksum = digest('md5', '1')
    assert(checksum == 'c4ca4238a0b923820dcc509a6f75849b') 
    checksum = digest('md5', 'a quick brown fox jumps over the lazy dog')
    assert(checksum == '040b5c6d12614d030027f359fcc481cf') 
    checksum = digest('md5', 'c4ca4238a0b923820dcc509a6f75849b')
    assert(checksum == '28c8edde3d61a0411511d3b1866f0636')
end

local function test_sha1()
    local checksum = digest('sha1', '1')
    assert(checksum == '356a192b7913b04c54574d18c28d46e6395428ab')
    checksum = digest('sha1', 'a quick brown fox jumps over the lazy dog')
    assert(checksum == 'a3b4cf96994a2c473fe9d9ce9ae7630c5bc9d898')
    checksum = digest('sha1', 'c4ca4238a0b923820dcc509a6f75849b')
    assert(checksum == '0937afa17f4dc08f3c0e5dc908158370ce64df86')
end

local function test_sha256()
    local checksum = digest('sha256', '1')
    assert(checksum == '6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b')
    checksum = digest('sha256', 'a quick brown fox jumps over the lazy dog')
    assert(checksum == '8f1ad6dfff1a460eb4ab78a5a7c3576209628ea200c1dbc70bda69938b401309')
    checksum = digest('sha256', 'c4ca4238a0b923820dcc509a6f75849b')
    assert(checksum == '08428467285068b426356b9b0d0ae1e80378d9137d5e559e5f8377dbd6dde29f')
end

local function test_sha512()
    local checksum = digest('sha512', '1')
    assert(checksum == '4dff4ea340f0a823f15d3f4f01ab62eae0e5da579ccb851f8db9dfe84c58b2b37b89903a740e1ee172da793a6e79d560e5f7f9bd058a12a280433ed6fa46510a')
    checksum = digest('sha512', 'a quick brown fox jumps over the lazy dog')
    assert(checksum == '3c689f11ae18630a757212648084467cf7a7b8b8719faf8cd823fe6938952aa106976ae5855c22ab8757ef081abed5001b17d5dd7cbc1e9f07fc432dde5abb8d')
    checksum = digest('sha512', 'c4ca4238a0b923820dcc509a6f75849b')
    assert(checksum == 'eb973daa92d0a9ce0f6136aba871d2969dd3526b8d90cf7c4bb41f3ed32af01b29e68ddc6b87ae60635feba97e42f86bf1a880f9dd5da717371e6c9007d403e4')
end

local function test_aes()
    -- key and iv must be 16 bytes length
    local key = digest('md5', 'this_surely_a_good_key', 'bin')
    local iv = digest('md5', 'this_might_be_a_good_iv', 'bin')
    local aes = crypto.createAES(key, iv)
    assert(type(aes) == 'userdata')
    
    local message = 'a quick brown fox jumps over the lazy dog'
    local encrypted = aes:encrypt(message)
    local decrypted = aes:decrypt(encrypted)
    assert(decrypted == message)
    
    encrypted = aes:encrypt(html_text)
    decrypted = aes:decrypt(encrypted)
    assert(decrypted == html_text)
end

local function test_rsa()
    --local pub, priv = crypto:gen_rsa_keypair()
    local client = crypto.createRSA()
    local server = crypto.createRSA()
    client:setPubKey(pubkey_text)
    server:setPriKey(prikey_text)
    
    local encrypted = client:pubEncrypt(html_text)
    local decrypted = server:priDecrypt(encrypted)
    assert(decrypted == html_text)
    
    encrypted = server:priEncrypt(html_text)
    decrypted = server:pubDecrypt(encrypted)
    assert(decrypted == html_text)
end

test_md5()
test_sha1()
test_sha256()
test_sha512()
test_aes()
test_rsa()

print('crypto passed')